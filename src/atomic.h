#ifndef ATOMIC_H
#define ATOMIC_H

#define atomic_cas(ptr, cmp, val) __sync_val_compare_and_swap(ptr, cmp, val)
#define atomic_swap(ptr, val) __atomic_exchange_n(ptr, val, __ATOMIC_ACQ_REL)

#endif /* end of include guard: ATOMIC_H */
