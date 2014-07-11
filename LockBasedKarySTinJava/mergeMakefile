TBB_INC=/people/cs/a/axr108820/tbb/src/tbb42_20131118oss/include 
TBB_LIB=/people/cs/a/axr108820/tbb/src/tbb42_20131118oss/build/linux_intel64_gcc_cc4.4.7_libc2.12_kernel2.6.32_release
GSL_INC=/people/cs/a/axr108820/gsl/installpath/include
GSL_LIB=/people/cs/a/axr108820/gsl/installpath/lib
TBBFLAGS=-I$(TBB_INC) -Wl,-rpath,$(TBB_LIB) -L$(TBB_LIB) -ltbb
GSLFLAGS=-I$(GSL_INC) -Wl,-rpath,$(GSL_LIB) -L$(GSL_LIB) -lgsl -lgslcblas -lm
CC=g++
CFLAGS= -O3  -lrt -g 
SRC= ./src/LockBasedIBst.c ./src/TestLockBasedIBst.c
OBJ= ./bin/LockBasedIBst.o
ibst: ./src/LockBasedIBst.c ./src/TestLockBasedIBst.c
	$(CC) $(CFLAGS) $(TBBFLAGS) $(GSLFLAGS)  -o $(OBJ) $(SRC)
clean:
	rm -rf ./bin/*.o

