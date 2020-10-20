#ifndef CILKPLUS_H
#define CILKPLUS_H

#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

#define fibril
#define fibril_t __attribute__((unused)) int
#define fibril_init(fp)
#define fibril_join(fp) cilk_sync

#define fibril_fork_nrt(fp, fn, ag)     cilk_spawn fn ag
#define fibril_fork_wrt(fp, rt, fn, ag) *rt = cilk_spawn fn ag

#define fibril_rt_init(n) do { \
	char nprocs[32]; \
	long l = sysconf(_SC_NPROCESSORS_ONLN); \
	if (l <= 0) l = 1; \
	if (l > INT_MAX) l = INT_MAX; \
	snprintf(nprocs, 32, "%d", (int) ((n > 0 && n < l) ? n : l)); \
	__cilkrts_set_param("nworkers", nprocs); \
	__cilkrts_set_param("stack size", "0x100000"); \
} while (0);
#define fibril_rt_exit() (__cilkrts_end_cilk())
#define fibril_rt_nprocs() (__cilkrts_get_nworkers())

#endif /* end of include guard: CILKPLUS_H */
