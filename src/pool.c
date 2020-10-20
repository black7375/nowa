#include <stdlib.h>
#include <sys/mman.h>
#include "safe.h"
#include "sync.h"
#include "mutex.h"
#include "param.h"
#include "stats.h"
#include "pool.h"

#ifndef POOL_GLOBAL_SIZE
#define POOL_GLOBAL_SIZE (2048 - 3)
#endif

#define POOL_LOCAL_COUNT 8
#define POOL_LOCAL_SIZE ((256 / POOL_LOCAL_COUNT) - 3)

#ifndef POOL_PRIVATE_SIZE
#define POOL_PRIVATE_SIZE 7
#endif

#ifndef POOL_CACHE_SIZE
#define POOL_CACHE_SIZE 4
#endif



static inline void *pool_alloc()
{
  void *stack;

  SAFE_RZCALL(posix_memalign(&stack, PARAM_PAGE_SIZE, PARAM_STACK_SIZE));
  STATS_INC(N_STACKS, 1);
#ifdef FIBRIL_STATS
  SAFE_NNCALL(mprotect(stack, PARAM_STACK_SIZE, PROT_NONE));
#endif

  return stack;
}

static inline void pool_free(void *stack)
{
  free(stack);
  STATS_DEC(N_STACKS, 1);
}



#ifdef POOL_WAIT_FREE


static __thread struct {
  volatile size_t pos;
  void *buf[POOL_PRIVATE_SIZE];
} _pp __attribute__((aligned(128)));

typedef struct stack {
  struct stack *next;
} stack_t;

#ifdef POOL_LOCAL_POOLS
typedef struct stackptr {
  stack_t *top;
} __attribute__((aligned(128))) stackptr_t;

stackptr_t _pl[POOL_LOCAL_COUNT];
#endif

//static stack_t *_pg __attribute__((aligned(128)));
//static stack_t *_heap __attribute__((aligned(128)));
static stack_t *_pg;
static stack_t *_heap;


#define ADDRESS_BITS 48
#define AVAILABLE_MASK (PARAM_PAGE_SIZE - 1)
#define ADDRESS_MASK (((1UL << ADDRESS_BITS) - 1) & (~AVAILABLE_MASK))

static inline stack_t *fold_pointer(stack_t *p, uint16_t c, uint16_t a)
{
  SAFE_ASSERT(a <= AVAILABLE_MASK);
  return (stack_t*) ((uintptr_t) p | ((uintptr_t) c << ADDRESS_BITS) | (uintptr_t) a);
}

static inline uint16_t unfold_counter(stack_t *p)
{
  return (size_t) ((uintptr_t) p >> ADDRESS_BITS);
}

static inline uint16_t unfold_available(stack_t *p)
{
  return (size_t) ((uintptr_t) p & AVAILABLE_MASK);
}

static inline stack_t *unfold_pointer(stack_t *p)
{
  return (stack_t*) ((uintptr_t) p & ADDRESS_MASK);
}

#undef ADDRESS_BITS
#undef AVAILABLE_MASK
#undef ADDRESS_MASK


static inline void* treiber_stack_pop(stack_t **top) {
  stack_t *stack;
  stack_t *next;
  stack_t *head;

  head = fatomic_load(*top);
  do {
    stack = unfold_pointer(head);
    if (!stack)
      return NULL;
    next = fold_pointer(fatomic_load(stack->next), unfold_counter(head) + 1, unfold_available(head) - 1);
  } while (!fatomic_cas_e(*top, head, next, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));

  return stack;
}

static inline int treiber_stack_push(
    stack_t **top,
    stack_t *chunk,
    stack_t *tail,
    size_t pool_size,
    size_t chunk_size
    ) {
  size_t available;
  stack_t *next;
  stack_t *head;

  available = 0;
  head = fatomic_load(*top);
  do {
    fatomic_store_e(tail->next, unfold_pointer(head), __ATOMIC_RELAXED);
    available = unfold_available(head) + chunk_size;
    if (pool_size && available > pool_size)
      return 0;
    next = fold_pointer(chunk, unfold_counter(head) + 1, available);
  } while (!fatomic_cas_e(*top, head, next, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));

  return 1;
}

static inline int treiber_stack_swap(
    stack_t **top,
    stack_t **chunk,
    stack_t **tail,
    size_t pool_size,
    size_t *chunk_size
    ) {
  stack_t *next;
  stack_t *head;

  if (pool_size && *chunk_size > pool_size)
    return 0;

  head = fatomic_load(*top);
#if 1
  do {
    next = fold_pointer(*chunk, unfold_counter(head) + 1, *chunk_size);
  } while (!fatomic_cas_e(*top, head, next, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE));
#else
  next = fold_pointer(*chunk, unfold_counter(head) + 1, *chunk_size);
  head = fatomic_swap(*top, next);
#endif

  *chunk = unfold_pointer(head);
  *chunk_size = unfold_available(head);
  for (*tail = *chunk; *tail && (*tail)->next; *tail = (*tail)->next) { }

  return 1;
}

void *pool_take()
{
  void *stack = NULL;

  if (_pp.pos > 0) {
    stack = _pp.buf[--_pp.pos];
    goto POOL_TAKE_SAFE_RETURN;
  }

#ifdef POOL_LOCAL_POOLS
  int idx = _tid / (256 / POOL_LOCAL_COUNT);
  stack = treiber_stack_pop(&(_pl[idx].top));
  if (stack)
    goto POOL_TAKE_SAFE_RETURN;
#endif

  stack = treiber_stack_pop(&_pg);
  if (stack)
    goto POOL_TAKE_SAFE_RETURN;

  stack = treiber_stack_pop(&_heap);
  if (stack)
    goto POOL_TAKE_SAFE_RETURN;

  stack = pool_alloc();

POOL_TAKE_SAFE_RETURN:
  SAFE_ASSERT(stack);
  return stack;
}

void pool_put(void *stack)
{
  SAFE_ASSERT(stack);

  while (_pp.pos >= POOL_PRIVATE_SIZE) {
    stack_t *chunk = _pp.buf[--_pp.pos];
    stack_t *tail = chunk;
    size_t chunk_size = 1;
    while (_pp.pos > POOL_CACHE_SIZE) {
      fatomic_store_e(tail->next, _pp.buf[--_pp.pos], __ATOMIC_RELAXED);
      tail = tail->next;
      chunk_size++;
    }

#ifdef POOL_LOCAL_POOLS
    int idx = _tid / (256 / POOL_LOCAL_COUNT);
    if (treiber_stack_push(&(_pl[idx].top), chunk, tail, POOL_LOCAL_SIZE, chunk_size))
      break;
    tail->next = NULL;
    treiber_stack_swap(&(_pl[idx].top), &chunk, &tail, POOL_LOCAL_SIZE, &chunk_size);
    if (!chunk_size)
      break;
#endif

    if (treiber_stack_push(&_pg, chunk, tail, POOL_GLOBAL_SIZE, chunk_size))
      break;

    tail->next = NULL;
    for (stack_t *current = chunk; current != NULL; current = current->next) {
      void *addr = (void*) ((char*) current + PARAM_PAGE_SIZE);
      SAFE_NNCALL(madvise(addr, PARAM_STACK_SIZE - PARAM_PAGE_SIZE, MADV_FREE));
    }

    treiber_stack_push(&_heap, chunk, tail, 0, 0);
  }

  /** Invariant: we always put stack into private pool. */
  _pp.buf[_pp.pos++] = stack;
}

#else


static struct {
  mutex_t * volatile lock;
  size_t volatile avail;
  void * buff[POOL_GLOBAL_SIZE];
} _pg __attribute__((aligned(128)));

#ifdef POOL_LOCAL_POOLS
struct {
  mutex_t * volatile lock;
  size_t volatile avail;
  void * buff[POOL_LOCAL_SIZE];
} __attribute__((aligned(128))) _pl[POOL_LOCAL_COUNT];
#endif

static __thread struct {
  size_t volatile avail;
  void * buff[POOL_PRIVATE_SIZE];
} _pp __attribute__((aligned(128)));

/**
 * Take a stack from the pool or allocate from heap if the pool is empty.
 * @return Return a stack or NULL if the pool has reached its limit.
 */
void * pool_take()
{
  void * stack = NULL;

  /** Take a stack from the available stacks. */
  if (_pp.avail > 0) {
    stack = _pp.buff[--_pp.avail];
    goto POOL_TAKE_SAFE_RETURN;
  }

  /** Take a stack from the parent pool. */
#ifdef POOL_LOCAL_POOLS
  //int idx = _tid % POOL_LOCAL_COUNT;
  int idx = _tid / (256 / POOL_LOCAL_COUNT);

  if (_pl[idx].avail > 0) {
    mutex_t mutex;
    mutex_lock(&_pl[idx].lock, &mutex);

    if (_pl[idx].avail > 0) {
      stack = _pl[idx].buff[--_pl[idx].avail];
      mutex_unlock(&_pl[idx].lock, &mutex);
      goto POOL_TAKE_SAFE_RETURN;
    }

    mutex_unlock(&_pl[idx].lock, &mutex);
  }
#endif

  if (_pg.avail > 0) {
    mutex_t mutex;
    mutex_lock(&_pg.lock, &mutex);

    if (_pg.avail > 0) {
      stack = _pg.buff[--_pg.avail];
      mutex_unlock(&_pg.lock, &mutex);
      goto POOL_TAKE_SAFE_RETURN;
    }

    mutex_unlock(&_pg.lock, &mutex);
  }

  stack = pool_alloc();

POOL_TAKE_SAFE_RETURN:
  SAFE_ASSERT(stack);
  return stack;
}

/**
 * Put a stack back into pool.
 * @param p The pool to put back into.
 * @param stack The stack to put back.
 */
void pool_put(void * stack)
{
  SAFE_ASSERT(stack);

  /** If local pool does not have space, */
  if (_pp.avail >= POOL_PRIVATE_SIZE) {
    /** Try moving stacks to parent pool. */
#ifdef POOL_LOCAL_POOLS
    int idx = _tid / (256 / POOL_LOCAL_COUNT);

    mutex_t mutex;
    mutex_lock(&_pl[idx].lock, &mutex);

    if (_pl[idx].avail >= POOL_LOCAL_SIZE - POOL_CACHE_SIZE && _pg.avail < POOL_GLOBAL_SIZE) {
      mutex_t mutex;
      mutex_lock(&_pg.lock, &mutex);

      /** Keep only POOL_LOCAL_SIZE / 2 stacks. */
      while (_pl[idx].avail > (POOL_LOCAL_SIZE / 2) && _pg.avail < POOL_GLOBAL_SIZE) {
        _pg.buff[_pg.avail++] = _pl[idx].buff[--_pl[idx].avail];
      }

      mutex_unlock(&_pg.lock, &mutex);
    }

    /** Keep only POOL_CACHE_SIZE stacks. */
    while (_pp.avail > POOL_CACHE_SIZE && _pl[idx].avail < POOL_LOCAL_SIZE) {
      _pl[idx].buff[_pl[idx].avail++] = _pp.buff[--_pp.avail];
    }

    mutex_unlock(&_pl[idx].lock, &mutex);

#else

    if (_pg.avail < POOL_GLOBAL_SIZE) {
      mutex_t mutex;
      mutex_lock(&_pg.lock, &mutex);

      /** Keep only POOL_CACHE_SIZE stacks. */
      while (_pp.avail > POOL_CACHE_SIZE && _pg.avail < POOL_GLOBAL_SIZE) {
        _pg.buff[_pg.avail++] = _pp.buff[--_pp.avail];
      }

      mutex_unlock(&_pg.lock, &mutex);
    }
#endif

    /** Free local pool for space. */
    while (_pp.avail >= POOL_PRIVATE_SIZE) {
      pool_free(_pp.buff[--_pp.avail]);
    }
  }

  /** Invariant: we always put stack into private pool. */
  _pp.buff[_pp.avail++] = stack;
}

#endif
