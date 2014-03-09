#include"header.h"
int NUM_OF_THREADS;
int findPercent;
int insertPercent;
int deletePercent;
unsigned long iterations;
unsigned long keyRange;
tbb::atomic<int> totalInserted;
tbb::atomic<int> totalDeleted;

void *operateOnTree(void *threadId)
{
  int chooseOperation;
  srand((long) threadId);
  for(int i=0;i<iterations;i++)
  {
    chooseOperation = rand()%100;
    if(chooseOperation < findPercent)
    {
      lookup(rand()%keyRange + 1);
    }
    else if (chooseOperation < insertPercent)
    {
      totalInserted.fetch_and_add(insert(rand()%keyRange + 1,rand()%keyRange + 1));
    }
    else
    {
      totalDeleted.fetch_and_add(remove(rand()%keyRange + 1));
    }
  }
}

int main(int argc, char *argv[])
{
  NUM_OF_THREADS = atoi(argv[1]);
  findPercent = atoi(argv[2]);
  insertPercent= findPercent + atoi(argv[3]);
  deletePercent = insertPercent + atoi(argv[4]);
  iterations = (unsigned long) pow(2,atoi(argv[5]));
  keyRange = (unsigned long) pow(2,atoi(argv[6]));
  printf("%d\t%d\t%d\t%d\t%d\t%d\n",NUM_OF_THREADS,findPercent,insertPercent,deletePercent,iterations,keyRange);

  createHeadNodes();
  for(int i=0;i<keyRange/2;i++)
  {
    insert(rand()%keyRange + 1,rand()%keyRange + 1);
  }
  printKeys();
  printf("Initial Size: %lu\n",size());
  pthread_t threadArray[NUM_OF_THREADS];
  for(int i=0;i<NUM_OF_THREADS;i++)
  {
    pthread_create(&threadArray[i], NULL, operateOnTree, (void *)i);
  }
  for(int i=0;i<NUM_OF_THREADS;i++)
  {
    pthread_join(threadArray[i], NULL);
  }
  printf("Final Size: %lu\n",size());
  printf("Inserted %lu\tDeleted %lu\n",(int) totalInserted,(int) totalDeleted);
  printKeys();
  pthread_exit(NULL);
}
