#ifndef OPENMP_H
#define OPENMP_H

#include <omp.h>
#include <thread>
#include <functional>

#define _fibril_expand(...) \
  _fibril_expand_(_fibril_nth(__VA_ARGS__), ## __VA_ARGS__)
#define _fibril_expand_(n, ...) \
  _fibril_concat(_fibril_expand_, n)(__VA_ARGS__)
#define _fibril_expand_16(...) __VA_ARGS__
#define _fibril_expand_15(...) __VA_ARGS__
#define _fibril_expand_14(...) __VA_ARGS__
#define _fibril_expand_13(...) __VA_ARGS__
#define _fibril_expand_12(...) __VA_ARGS__
#define _fibril_expand_11(...) __VA_ARGS__
#define _fibril_expand_10(...) __VA_ARGS__
#define _fibril_expand_9( ...) __VA_ARGS__
#define _fibril_expand_8( ...) __VA_ARGS__
#define _fibril_expand_7( ...) __VA_ARGS__
#define _fibril_expand_6( ...) __VA_ARGS__
#define _fibril_expand_5( ...) __VA_ARGS__
#define _fibril_expand_4( ...) __VA_ARGS__
#define _fibril_expand_3( ...) __VA_ARGS__
#define _fibril_expand_2( ...) __VA_ARGS__
#define _fibril_expand_1( ...) __VA_ARGS__
#define _fibril_expand_0()


template<class F, class ...As>
__attribute__((always_inline))
inline static void _omp_fork_nrt(F f, As... as) {
#pragma omp task untied
//#pragma omp task
	{
		f(as...);
	}
}

template<class F, class R, class ...As>
__attribute__((always_inline))
inline static void _omp_fork_wrt(F f, R r, As... as) {
#pragma omp task untied
//#pragma omp task
	{
		*r = f(as...);
	}
}

static int NTHREADS;
int fibril_rt_nprocs() {
	return (NTHREADS) ? NTHREADS : std::thread::hardware_concurrency();
}

__attribute__((always_inline))
inline static void _omp_init(int n, std::function<void(void)> f) {
	int nprocs = std::thread::hardware_concurrency();
	if (n > 0 && n < nprocs) {
		NTHREADS = n;
	} else {
		NTHREADS = nprocs;
	}
#pragma omp parallel sections num_threads(NTHREADS) default(shared)
	{
		f();
	}
}


#define fibril
#define fibril_t __attribute__((unused)) int
#define fibril_init(fp)

__attribute__((always_inline))
inline static void fibril_join(__attribute__((unused)) fibril_t *f) {
#pragma omp taskwait
}

#define fibril_fork_nrt(fp, fn, ag) _omp_fork_nrt(fn, _fibril_expand ag)
#define fibril_fork_wrt(fp, rtp, fn, ag) _omp_fork_wrt(fn, rtp, _fibril_expand ag)

#define fibril_rt_init(n) _omp_init(n, [&]() {
#define fibril_rt_exit() })

#endif /* end of include guard: OPENMP_H */
