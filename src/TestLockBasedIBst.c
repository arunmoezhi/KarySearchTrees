#include"header.h"
int NUM_OF_THREADS;
int findPercent;
int insertPercent;
int deletePercent;
unsigned long iterations;
unsigned long keyRange;
double* timeArray;
double MOPS;

struct threadArgs
{
  int threadId;
  unsigned int seed;
};

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

void *operateOnTree(void* tArgs)
{
  struct timespec s,e;
  int chooseOperation;
  unsigned int seed;
  int threadId;
  threadId = ((struct threadArgs*) tArgs)->threadId;
  seed = ((struct threadArgs*) tArgs)->seed;
  unsigned int readCount=0;
  unsigned int successfulReadCount=0;
  clock_gettime(CLOCK_REALTIME,&s);
  for(int i=0;i<iterations;i++)
  {
    chooseOperation = rand_r(&seed)%100;
    if(chooseOperation < findPercent)
    {
      readCount++;
      successfulReadCount += lookup(rand_r(&seed)%keyRange + 1);
    }
    else if (chooseOperation < insertPercent)
    {
      #ifdef VALUE_PRESENT
      insert(rand_r(&seed)%keyRange + 1, 1);
      #else
      insert(rand_r(&seed)%keyRange + 1);
      #endif
    }
    else
    {
      remove(rand_r(&seed)%keyRange + 1);
    }
  }
  clock_gettime(CLOCK_REALTIME,&e);
  timeArray[threadId] = (double) (diff(s,e).tv_sec * 1000000000 + diff(s,e).tv_nsec)/1000;
  //printf("%lu,%lu\n",readCount,successfulReadCount);
}

int main(int argc, char *argv[])
{
  struct threadArgs** tArgs;
  double totalTime=0;
  double avgTime=0;
  NUM_OF_THREADS = atoi(argv[1]);
  findPercent = atoi(argv[2]);
  insertPercent= findPercent + atoi(argv[3]);
  deletePercent = insertPercent + atoi(argv[4]);
  iterations = (unsigned long) pow(2,atoi(argv[5]));
  keyRange = (unsigned long) pow(2,atoi(argv[6]));
  timeArray = (double*)malloc(NUM_OF_THREADS * sizeof(double));
  tArgs = (struct threadArgs**) malloc(NUM_OF_THREADS * sizeof(struct threadArgs*));
  unsigned int seed = 1;
  srand(seed);
  createHeadNodes();
  
  int i=0;
  while(i < keyRange/2)
  {
    #ifdef VALUE_PRESENT
    i += insert(rand_r(&seed)%keyRange + 1,1);
    #else
    i += insert(rand_r(&seed)%keyRange + 1);
    #endif
  }
  #ifdef DEBUG_ON
  printKeys();
  printf("Initial Size: %lu\n",size());
  #endif
  pthread_t threadArray[NUM_OF_THREADS];
  for(int i=0;i<NUM_OF_THREADS;i++)
  {
    tArgs[i] = (struct threadArgs*) malloc(sizeof(struct threadArgs));
    tArgs[i]->threadId = i;
    tArgs[i]->seed = rand();
  }

  for(int i=0;i<NUM_OF_THREADS;i++)
  {
    pthread_create(&threadArray[i], NULL, operateOnTree, (void*) tArgs[i] );
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
