# You have to make sure CLAPACKLIBS is set...

CXXFLAGS = -msse -Wall -I.. \
      -DKALDI_DOUBLEPRECISION=0 -msse2 -DHAVE_POSIX_MEMALIGN \
      -Wno-sign-compare \
      -DHAVE_EXECINFO_H=1 -rdynamic -DHAVE_CXXABI_H \
      -DUSE_KALDI_SVD -DHAVE_OPENBLAS -I $(OPENBLASROOT)/include \
      -I ../../tools/openfst/include \
      $(EXTRA_CXXFLAGS) \
      -g # -O0 -DKALDI_PARANOID 

LDFLAGS = -rdynamic
LDLIBS = $(EXTRA_LDLIBS) ../../tools/openfst/lib/libfst.a -ldl $(OPENBLASLIBS) -lm -lpthread
CC = g++
CXX = g++
AR = ar
AS = as
RANLIB = ranlib
