VPATH = $(top_srcdir)/test

AM_CPPFLAGS = -I$(top_srcdir)/fibril/build/include/ -DBENCHMARK
AM_LDFLAGS = -L$(top_srcdir)/fibril/build/lib/ -lfibril

check_PROGRAMS = \
                 cholesky \
                 fft \
                 fib \
                 heat \
                 integrate \
                 knapsack \
                 lu \
                 matmul \
                 nqueens \
                 quicksort \
                 rectmul \
                 strassen

cholesky_LDADD = -lm
fft_LDADD = -lm
heat_LDADD = -lm
lu_LDADD = -lm
strassen_LDADD = -lm

TESTS = $(check_PROGRAMS)
