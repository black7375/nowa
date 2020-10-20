#include <stddef.h>
#include "sync.h"
#include "debug.h"
#include "deque.h"

__thread deque_t fibrili_deq;

#ifdef DEQUE_USE_THE
struct _fibril_t * deque_steal(deque_t * deq)
{
  if (deq->head >= deq->tail) return NULL;

  sync_lock(deq->lock);

  int head = deq->head++;

  sync_fence();

  if (head >= deq->tail) {
    deq->head--;
    sync_unlock(deq->lock);

    return NULL;
  }

  struct _fibril_t * frptr = deq->buff[head];

  sync_unlock(deq->lock);
  return frptr;
}

#else

struct _fibril_t * deque_steal(deque_t * deq)
{
  uint64_t head = fatomic_load_e(deq->head, __ATOMIC_RELAXED);

start:
  if (fatomic_load(deq->tail) <= head)
    return NULL;

  void *frptr = deq->buff[head % DEQUE_SIZE];

  if (!fatomic_cas_e(deq->head, head, head + 1, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE))
    goto start;

  return frptr;
}
#endif

