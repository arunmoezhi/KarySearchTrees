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
#define UINTPTR_MAX_XOR_WITH_1 (uintptr_t) (UINTPTR_MAX ^ 1)
#define K 2
#define NUM_OF_KEYS_IN_A_NODE K-1
#define NUM_OF_CHILDREN_FOR_A_NODE K

//#define DEBUG_ON

void createHeadNodes();
unsigned long lookup(unsigned long);
bool insert(unsigned long , unsigned long );
bool remove(unsigned long);
unsigned long size();
void printKeys();
