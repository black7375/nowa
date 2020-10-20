#ifndef FIBRILI_H
#define FIBRILI_H

#include <stdint.h>


#ifndef DEQUE_SIZE
#define DEQUE_SIZE (1024)
#endif

struct _fibril_t {
  uint32_t count;
  uint32_t steals;
  int resumable;
  char unmapped;
  struct {
    void * btm;
    void * top;
    void * ptr;
  } stack;
  void * pc;
};


#ifdef DEQUE_USE_THE
extern __thread struct _fibrili_deque_t {
  char lock;
  int  head;
  int  tail;
  void * stack;
  void * buff[DEQUE_SIZE];
} fibrili_deq;

#else

extern __thread struct _fibrili_deque_t {
  uint64_t head;
  uint64_t tail;
  void * stack;
  void * buff[DEQUE_SIZE];
} fibrili_deq;
#endif


#if defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ > 7

#define fibrili_fence() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define fibrili_lock(l) do { \
  __asm__ ( "pause" : : : "memory" ); \
} while (__atomic_test_and_set(&(l), __ATOMIC_ACQUIRE))
#define fibrili_unlock(l) __atomic_clear(&(l), __ATOMIC_RELEASE)

#else
#if defined(__x86_64__) || defined(_M_X64_)

#define fibrili_fence() __sync_synchronize()
#define fibrili_lock(l) do { \
  __asm__ ( "pause" ::: "memory" ); \
} while (__sync_lock_test_and_set(&(l), 1))
#define fibrili_unlock(l) __sync_lock_release(&(l))

#endif
#endif


#define fatomic_load(val)        __atomic_load_n(&(val), __ATOMIC_ACQUIRE)
#define fatomic_store(val, n)      __atomic_store_n(&(val), n, __ATOMIC_RELEASE)

#define fatomic_fadd(val, n)      __atomic_fetch_add(&(val), n, __ATOMIC_ACQ_REL)
#define fatomic_fsub(val, n)      __atomic_fetch_sub(&(val), n, __ATOMIC_ACQ_REL)
#define fatomic_addf(val, n)      __atomic_add_fetch(&(val), n, __ATOMIC_ACQ_REL)
#define fatomic_subf(val, n)      __atomic_sub_fetch(&(val), n, __ATOMIC_ACQ_REL)

#define fatomic_swap(val, n)      __atomic_exchange_n(&(val), n, __ATOMIC_ACQ_REL)

#define fatomic_cas(ptr, cmp, val)    __atomic_compare_exchange_n(&(ptr), &(cmp), val, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)


#define fatomic_load_e(val, m)        __atomic_load_n(&(val), m)
#define fatomic_store_e(val, n, m)      __atomic_store_n(&(val), n, m)

#define fatomic_fadd_e(val, n, m)      __atomic_fetch_add(&(val), n, m)
#define fatomic_fsub_e(val, n, m)      __atomic_fetch_sub(&(val), n, m)
#define fatomic_addf_e(val, n, m)      __atomic_add_fetch(&(val), n, m)
#define fatomic_subf_e(val, n, m)      __atomic_sub_fetch(&(val), n, m)

#define fatomic_swap_e(val, n, m)      __atomic_exchange_n(&(val), n, m)

#define fatomic_cas_e(ptr, cmp, val, ms, mf)    __atomic_compare_exchange_n(&(ptr), &(cmp), val, 0, ms, mf)


__attribute__((noinline)) extern
void fibrili_join(struct _fibril_t * frptr);
__attribute__((noreturn)) extern
void fibrili_resume(struct _fibril_t * frptr, uint32_t n);

#ifdef DEQUE_USE_THE
#define fibrili_push(frptr) do { \
  (frptr)->pc = __builtin_return_address(0); \
  fibrili_deq.buff[fibrili_deq.tail++] = (frptr); \
} while (0)

__attribute__((hot)) static
int fibrili_pop(void)
{
  int tail = fibrili_deq.tail;

  if (tail == 0) return 0;

  fibrili_deq.tail = --tail;

  fibrili_fence();

  if (fibrili_deq.head > tail) {
    fibrili_deq.tail = tail + 1;

    fibrili_lock(fibrili_deq.lock);

    if (fibrili_deq.head > tail) {
      fibrili_deq.head = 0;
      fibrili_deq.tail = 0;

      fibrili_unlock(fibrili_deq.lock);
      return 0;
    }

    fibrili_deq.tail = tail;
    fibrili_unlock(fibrili_deq.lock);
  }

  return 1;
}

#else

#define fibrili_push(frptr) do { \
  (frptr)->pc = __builtin_return_address(0); \
  uint64_t tail = fatomic_load_e(fibrili_deq.tail, __ATOMIC_ACQUIRE); \
  fibrili_deq.buff[tail % DEQUE_SIZE] = (frptr); \
  fatomic_store_e(fibrili_deq.tail, tail + 1, __ATOMIC_RELEASE); \
} while (0)

__attribute__((hot)) static
int fibrili_pop(void)
{
  uint64_t tail = fatomic_subf(fibrili_deq.tail, 1);
  uint64_t head = fatomic_load(fibrili_deq.head);

  if (tail > head)
    return 1;

  if (head > tail) {
    fatomic_store_e(fibrili_deq.tail, head, __ATOMIC_RELAXED);
    return 0;
  }

  int res = fatomic_cas_e(fibrili_deq.head, head, head + 1, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
  fatomic_store_e(fibrili_deq.tail, tail + 1, __ATOMIC_RELAXED);

  return res;
}
#endif

#define fibrili_membar(call) do { \
  call; \
  __asm__ ( "nop" : : : "rbx", "r12", "r13", "r14", "r15", "memory" ); \
} while (0)

#endif /* end of include guard: FIBRILI_H */
