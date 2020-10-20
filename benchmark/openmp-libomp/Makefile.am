include $(srcdir)/../Makefile.am
AM_CFLAGS = -std=c++17
AM_CPPFLAGS += -x c++ -DFIBRIL_OPENMP -fopenmp -fpermissive
#AM_LDFLAGS += -lomp -L/usr/lib -lstdc++
AM_LDFLAGS += /usr/lib/llvm-11/lib/libomp-11.so.5 -lstdc++
