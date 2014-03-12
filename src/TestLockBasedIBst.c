#include"header.h"
#define ATOMIC_COUNTERS
int NUM_OF_THREADS;
int findPercent;
int insertPercent;
int deletePercent;
unsigned long iterations;
unsigned long keyRange;
double* timeArray;
double MOPS;
#ifdef ATOMIC_COUNTERS
tbb::atomic<int> totalInserted;
tbb::atomic<int> totalDeleted;
#endif

struct timespec diff(timespec start, timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) 
	{
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else 
	{
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

void *operateOnTree(void* threadId)
{
  struct timespec s,e;
  int chooseOperation;
  srand((long) threadId);
  clock_gettime(CLOCK_REALTIME,&s);
  for(int i=0;i<iterations;i++)
  {
    chooseOperation = rand()%100;
    if(chooseOperation < findPercent)
    {
      lookup(rand()%keyRange + 1);
    }
    else if (chooseOperation < insertPercent)
    {
      #ifdef ATOMIC_COUNTERS
      totalInserted.fetch_and_add(insert(rand()%keyRange + 1,rand()%keyRange + 1));
      #else
      insert(rand()%keyRange + 1,rand()%keyRange + 1);
      #endif
    }
    else
    {
      #ifdef ATOMIC_COUNTERS
      totalDeleted.fetch_and_add(remove(rand()%keyRange + 1));
      #else
      remove(rand()%keyRange + 1);
      #endif
    }
  }
  clock_gettime(CLOCK_REALTIME,&e);
  timeArray[(intptr_t) threadId] = (double) (diff(s,e).tv_sec * 1000000000 + diff(s,e).tv_nsec)/1000;
}

int main(int argc, char *argv[])
{
  double totalTime=0;
  double avgTime=0;
  NUM_OF_THREADS = atoi(argv[1]);
  findPercent = atoi(argv[2]);
  insertPercent= findPercent + atoi(argv[3]);
  deletePercent = insertPercent + atoi(argv[4]);
  iterations = (unsigned long) pow(2,atoi(argv[5]));
  keyRange = (unsigned long) pow(2,atoi(argv[6]));
  timeArray = (double*)malloc(NUM_OF_THREADS * sizeof(double));
  createHeadNodes();
  for(int i=0;i<keyRange/2;i++)
  {
    insert(rand()%keyRange + 1,rand()%keyRange + 1);
  }
  #ifdef DEBUG_ON
  printKeys();
  printf("Initial Size: %lu\n",size());
  #endif
  pthread_t threadArray[NUM_OF_THREADS];
  for(int i=0;i<NUM_OF_THREADS;i++)
  {
    pthread_create(&threadArray[i], NULL, operateOnTree, (void*) i);
  }
  for(int i=0;i<NUM_OF_THREADS;i++)
  {
    pthread_join(threadArray[i], NULL);
  }
  #ifdef DEBUG_ON
  printf("Final Size: %lu\n",size());
  printf("Inserted %d\tDeleted %d\n",(int) totalInserted,(int) totalDeleted);
  printKeys();
  #endif
  for(int i=0;i<NUM_OF_THREADS;i++)
  {
    totalTime += timeArray[i];
  }
  avgTime = totalTime/NUM_OF_THREADS;
  MOPS = iterations*NUM_OF_THREADS/(avgTime);
  printf("k%d;%d-%d-%d;%d;%ld;%.2f;%.2f\n",atoi(argv[6]),findPercent,(insertPercent-findPercent),(deletePercent-insertPercent),NUM_OF_THREADS,size(),avgTime/1000,MOPS);
  pthread_exit(NULL);
}
