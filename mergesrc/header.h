#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<stdbool.h>
#include<limits.h>
#include<math.h>
#define __STDC_LIMIT_MACROS
#include<stdint.h>
#include<tbb/atomic.h>

#define K 2
#define NUM_OF_KEYS_IN_A_NODE K-1
#define NUM_OF_CHILDREN_FOR_A_NODE K

void createHeadNodes();
unsigned long lookup(unsigned long);
bool insert(unsigned long , unsigned long );
bool remove(unsigned long);
unsigned long size();
void printKeys();
