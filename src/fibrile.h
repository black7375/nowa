#ifndef FIBRILE_H
#define FIBRILE_H

#ifdef __cplusplus
extern "C" {
#endif


#include "fibrili.h"

/** fibril. */
#define fibril __attribute__((optimize("no-omit-frame-pointer")))

/** fibril_t. */
typedef struct _fibril_t fibril_t;

/** fibril_init. */
__attribute__((always_inline)) extern inline
void fibril_init(fibril_t * frptr)
{
  register void * rbp asm ("rbp");
  register void * rsp asm ("rsp");

  frptr->unmapped = 0;
  frptr->count = 0;
  frptr->steals = 0;
  frptr->resumable = 0;
  frptr->stack.btm = rbp;
  frptr->stack.top = rsp;
  frptr->stack.ptr = fibrili_deq.stack;
}

/** fibril_join. */
__attribute__((always_inline)) extern inline
void fibril_join(fibril_t * frptr)
{
  if (frptr->steals > 0) {
    fibrili_membar(fibrili_join(frptr));
    frptr->steals = 0;
    frptr->resumable = 0;
  }
}

#include "fork.h"


#ifdef __cplusplus

/** _fibril_fork_nrt. */
#define fibril_fork_nrt(fp, fn, ag) do { \
  auto _fibril_##fn##_fork = [](_fibril_defs ag fibril_t * f) __attribute__((noinline, hot, optimize(3))) { \
    fibrili_push(f); \
    fn(_fibril_args ag); \
    if (!fibrili_pop()) fibrili_resume(f, 1); \
  }; \
  fibrili_membar(_fibril_##fn##_fork(_fibril_expand ag fp)); \
} while (0)

/** _fibril_fork_wrt. */
#define fibril_fork_wrt(fp, rtp, fn, ag) do { \
  auto _fibril_##fn##_fork = [](_fibril_defs ag fibril_t * f, __typeof__(rtp) p) __attribute__((noinline, hot, optimize(3))) { \
    fibrili_push(f); \
    *p = fn(_fibril_args ag); \
    if (!fibrili_pop()) fibrili_resume(f, 1); \
  }; \
  fibrili_membar(_fibril_##fn##_fork(_fibril_expand ag fp, rtp)); \
} while (0)

#else

/** _fibril_fork_nrt. */
#define fibril_fork_nrt(fp, fn, ag) do { \
  __attribute__((noinline, hot, optimize(3))) \
  void _fibril_##fn##_fork(_fibril_defs ag fibril_t * f) { \
    fibrili_push(f); \
    fn(_fibril_args ag); \
    if (!fibrili_pop()) fibrili_resume(f, 1); \
  } \
  fibrili_membar(_fibril_##fn##_fork(_fibril_expand ag fp)); \
} while (0)

/** _fibril_fork_wrt. */
#define fibril_fork_wrt(fp, rtp, fn, ag) do { \
  __attribute__((noinline, hot, optimize(3))) \
  void _fibril_##fn##_fork(_fibril_defs ag fibril_t * f, __typeof__(rtp) p) { \
    fibrili_push(f); \
    *p = fn(_fibril_args ag); \
    if (!fibrili_pop()) fibrili_resume(f, 1); \
  } \
  fibrili_membar(_fibril_##fn##_fork(_fibril_expand ag fp, rtp)); \
} while (0)

#endif


extern int fibril_rt_init(int nprocs);
extern int fibril_rt_exit();
extern int fibril_rt_nprocs();

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: FIBRILE_H */
