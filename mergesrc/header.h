#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<stdbool.h>
#include<limits.h>
#include<math.h>
#include<time.h>
#define __STDC_LIMIT_MACROS
#include<stdint.h>
#include<tbb/atomic.h>
#include<gsl/gsl_rng.h>
#include<gsl/gsl_randist.h>
//#define UINTPTR_MAX_XOR_WITH_1 0xFFFFFFFFFFFFFFFE
#define UINTPTR_MAX_XOR_WITH_1 (uintptr_t) (UINTPTR_MAX ^ 1)
#define UINTPTR_MAX_XOR_WITH_3 (uintptr_t) (UINTPTR_MAX ^ 3)
#define USE_GSL
//#define VALUE_PRESENT
//#define DEBUG_ON

void createHeadNodes();
unsigned long lookup(unsigned long);
#ifdef VALUE_PRESENT
bool insert(unsigned long, unsigned long );
#else
bool insert(unsigned long);
#endif
bool remove(unsigned long);
unsigned long size();
void printKeys();
