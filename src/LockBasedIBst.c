#include"header.h"

struct node
{
  unsigned long key;
  #ifdef VALUE_PRESENT
  unsigned long value;
  #endif
  tbb::atomic<struct node*> lChild;    //format <address,lockbit>
  tbb::atomic<struct node*> rChild;    //format <address,lockbit>
};

struct node* grandParentHead=NULL;
struct node* parentHead=NULL;
unsigned long numOfNodes;

#ifdef VALUE_PRESENT
struct node* newLeafNode(unsigned long key, unsigned long value)
{
  struct node* node = (struct node*) malloc(sizeof(struct node));
  node->key = key;
  node->value = value;
  node->lChild = NULL;
  node->rChild = NULL;
  return(node);
}

void createHeadNodes()
{
  grandParentHead = newLeafNode(ULONG_MAX, ULONG_MAX);
  grandParentHead->lChild = newLeafNode(ULONG_MAX, ULONG_MAX);
  parentHead = grandParentHead->lChild;
}
#else
struct node* newLeafNode(unsigned long key)
{
  struct node* node = (struct node*) malloc(sizeof(struct node));
  node->key = key;
  node->lChild = NULL; 
  node->rChild = NULL;
  return(node);
}

void createHeadNodes()
{
  grandParentHead = newLeafNode(ULONG_MAX);
  grandParentHead->lChild = newLeafNode(ULONG_MAX);
  parentHead = grandParentHead->lChild;
}
#endif

bool getLockBit(struct node* p)
{
  //printf("lockbit %lu\t%lu\n",p, (uintptr_t) p & 1);
  return (uintptr_t) p & 1;
}

//#define getAddress(p) ((struct node*) ((uintptr_t) (struct node*) (p) & UINTPTR_MAX_XOR_WITH_1))

static inline struct node* getAddress(struct node* p)
{
  //printf("address %lu\t%lu\n",p, (uintptr_t) p & UINTPTR_MAX_XOR_WITH_1);
  return (struct node*)((uintptr_t) p & UINTPTR_MAX_XOR_WITH_1);
}

struct node* setLockBit(struct node* p)
{
  return((struct node*) (((uintptr_t) p & UINTPTR_MAX_XOR_WITH_1) | 1));
}

struct node* unsetLockBit(struct node* p)
{
  return((struct node*) (((uintptr_t) p & UINTPTR_MAX_XOR_WITH_1) | 0));
}

bool lockLChild(struct node* parent)
{
  struct node* lChild;
  struct node* lockedLChild;

  lChild = parent->lChild;
  if(getLockBit(lChild))
  {
    return false;
  }
  lockedLChild = setLockBit(lChild);

  if(parent->lChild.compare_and_swap(lockedLChild,lChild) == lChild)
  {
    return true;
  }
  return false;
}

bool lockRChild(struct node* parent)
{
  struct node* rChild;
  struct node* lockedRChild;

  rChild = parent->rChild;
  if(getLockBit(rChild))
  {
    return false;
  }
  lockedRChild = setLockBit(rChild);

  if(parent->rChild.compare_and_swap(lockedRChild,rChild) == rChild)
  {
    return true;
  }
  return false;
}

void unlockLChild(struct node* parent)
{
  parent->lChild = unsetLockBit(parent->lChild);
}

void unlockRChild(struct node* parent)
{
  parent->rChild = unsetLockBit(parent->rChild);
}

unsigned long lookup(unsigned long target)
{
  struct node* node;
  unsigned long lastRightKey;
  struct node* lastRightNode;
  while(true)
  {
    node = grandParentHead;
    lastRightKey = node->key;
    lastRightNode = node;
    while( node != NULL ) //Loop until a child of a leaf node which is null is reached
    {
      if(target < node->key)
      {
        node = getAddress(node->lChild);
      }
      else if (target > node->key)
      {
        lastRightKey = node->key;
        lastRightNode = node;
        node = getAddress(node->rChild);
      }
      else
      {
        #ifdef VALUE_PRESENT
        return(node->value);
        #else
        return(1);
        #endif
      }
    }
    if(lastRightNode->key == lastRightKey)
    {
      return(0);
    }
  }
}

#ifdef VALUE_PRESENT
bool insert(unsigned long insertKey, unsigned long insertValue)
#else
bool insert(unsigned long insertKey)
#endif
{
  struct node* pnode;
  struct node* node;
  struct node* replaceNode;
  unsigned long lastRightKey;
  struct node* lastRightNode;
  
  while(true)
  {
    while(true)
    {
      pnode = grandParentHead;
      node = parentHead;
      replaceNode = NULL;
      lastRightKey = node->key;
      lastRightNode = node;

      while( node != NULL ) //Loop until a child of a leaf node which is null is reached
      {
        if(insertKey < node->key)
        {
          pnode = node;
          node = getAddress(node->lChild);
        }
        else if (insertKey > node->key)
        {
          lastRightKey = node->key;
          lastRightNode = node;
          pnode = node;
          node = getAddress(node->rChild);
        }
        else
        {
          #ifdef DEBUG_ON
          printf("Failed I%lu\t%lu\n",insertKey,getAddress(parentHead->lChild)->key);
          #endif
          return(false);
        }
      }
      if(lastRightNode->key == lastRightKey)
      {
        break;  
      }
    }

    if(!getLockBit(pnode)) //If locked restart
    {
      #ifdef VALUE_PRESENT
      replaceNode = newLeafNode(insertKey, insertValue);
      #else
      replaceNode = newLeafNode(insertKey);
      #endif

      if(insertKey < pnode->key) //left case
      {
        if(pnode->lChild.compare_and_swap(replaceNode,node) == node)
        {
          //free(node);
          #ifdef DEBUG_ON
          printf("Success I%lu\t%lu\n",insertKey,getAddress(parentHead->lChild)->key);
          #endif
          return(true);
        }
      }
      else                      //right case
      {
        if(pnode->rChild.compare_and_swap(replaceNode,node) == node)
        {
          //free(node);
          #ifdef DEBUG_ON
          printf("Success I%lu\t%lu\n",insertKey,getAddress(parentHead->lChild)->key);
          #endif
          return(true);
        }
      }
      free(replaceNode);
    }
  }
}

bool remove(unsigned long deleteKey)
{
  struct node* pnode;
  struct node* node;
  unsigned long lastRightKey;
  struct node* lastRightNode;
  bool keyFound;
  
  while(true)
  {
    while(true)
    {
      pnode = grandParentHead;
      node = parentHead;
      lastRightKey = node->key;
      lastRightNode = node;
      keyFound = false;

      while( node != NULL ) //Loop until a child of a leaf node which is null is reached
      {
        if(deleteKey < node->key)
        {
          pnode = node;
          node = getAddress(node->lChild);
        }
        else if (deleteKey > node->key)
        {
          lastRightKey = node->key;
          lastRightNode = node;
          pnode = node;
          node = getAddress(node->rChild);
        }
        else //key to be deleted is found
        {
          keyFound = true;
          break;
        }
      }
      if(keyFound)
      {
        break;
      }
      if(lastRightNode->key == lastRightKey)
      {
        break;
      }
    }
    if(node != NULL)
    {
      if(deleteKey == node->key)
      {
        if(getAddress(pnode->lChild) == node) //left case
        {
          if(lockLChild(pnode))
          {
						if(lockLChild(node))
						{
							if(lockRChild(node)) //all locks are obtained
							{
								if(getAddress(node->lChild) == NULL)
								{
									if(getAddress(node->rChild) == NULL) // 0 0 case
									{
										pnode->lChild = NULL;  
										#ifdef DEBUG_ON
										printf("Success SD%lu\t%lu\n",deleteKey,getAddress(parentHead->lChild)->key);
										#endif
										return(true);
									}
									else // 0 1 case
									{
										pnode->lChild = getAddress(node->rChild);
										#ifdef DEBUG_ON
										printf("Success SD%lu\t%lu\n",deleteKey,getAddress(parentHead->lChild)->key);
										#endif
										return(true);
									}
								}
								else
								{
									if(getAddress(node->rChild) == NULL) // 1 0 case
									{
										pnode->lChild = getAddress(node->lChild);
										#ifdef DEBUG_ON
										printf("Success SD%lu\t%lu\n",deleteKey,getAddress(parentHead->lChild)->key);
										#endif
										return(true);
									}
									else // 1 1 case
									{
										struct node* rpnode;
										struct node* rnode;
										struct node* lcrnode;
										rpnode = node;
										rnode = getAddress(node->rChild);
										lcrnode = getAddress(rnode->lChild);
										if(lcrnode != NULL)
										{
											while(lcrnode != NULL)
											{
												rpnode = rnode;
												rnode = lcrnode;
												lcrnode = getAddress(lcrnode->lChild);
											}
											if(lockLChild(rpnode))
											{
												if(lockRChild(rnode))
												{
													node->key = rnode->key;
													#ifdef VALUE_PRESENT
													node->value = rnode->value;
													#endif
													rpnode->lChild = getAddress(rnode->rChild);
													unlockLChild(pnode);
													unlockLChild(node);
													unlockRChild(node);
													#ifdef DEBUG_ON
													printf("Success CD%lu\t%lu\n",deleteKey,getAddress(parentHead->lChild)->key);
													#endif
													return(true);
												}
												else
												{
													unlockLChild(pnode);
													unlockLChild(node);
													unlockRChild(node);
													unlockLChild(rpnode);
												}
											}
											else
											{
												unlockLChild(pnode);
												unlockLChild(node);
												unlockRChild(node);
											}
										}
										else
										{
											if(lockRChild(rnode))
											{
												node->key = rnode->key;
												#ifdef VALUE_PRESENT
												node->value = rnode->value;
												#endif
												node->rChild = getAddress(rnode->rChild);
												unlockLChild(pnode);
												unlockLChild(node);
												#ifdef DEBUG_ON
												printf("Success CD%lu\t%lu\n",deleteKey,getAddress(parentHead->lChild)->key);
												#endif
												return(true);
											}
											else
											{
												unlockLChild(pnode);
												unlockLChild(node);
												unlockRChild(node);
											}
										}
									}
								}
							}
							else
							{
								unlockLChild(pnode);
								unlockLChild(node);
							}
						}
						else
						{
							unlockLChild(pnode);
						}
					}
				}
				else //right case
				{
					if(lockRChild(pnode))
          {
						if(lockLChild(node))
						{
							if(lockRChild(node)) //all locks are obtained
							{
								if(getAddress(node->lChild) == NULL)
								{
									if(getAddress(node->rChild) == NULL) // 0 0 case
									{
										pnode->rChild = NULL;  
										#ifdef DEBUG_ON
										printf("Success SD%lu\t%lu\n",deleteKey,getAddress(parentHead->lChild)->key);
										#endif
										return(true);
									}
									else // 0 1 case
									{
										pnode->rChild = getAddress(node->rChild);
										#ifdef DEBUG_ON
										printf("Success SD%lu\t%lu\n",deleteKey,getAddress(parentHead->lChild)->key);
										#endif
										return(true);
									}
								}
								else
								{
									if(getAddress(node->rChild) == NULL) // 1 0 case
									{
										pnode->rChild = getAddress(node->lChild);
										#ifdef DEBUG_ON
										printf("Success SD%lu\t%lu\n",deleteKey,getAddress(parentHead->lChild)->key);
										#endif
										return(true);
									}
									else // 1 1 case
									{
										struct node* rpnode;
										struct node* rnode;
										struct node* lcrnode;
										rpnode = node;
										rnode = getAddress(node->rChild);
										lcrnode = getAddress(rnode->lChild);
										if(lcrnode != NULL)
										{
											while(lcrnode != NULL)
											{
												rpnode = rnode;
												rnode = lcrnode;
												lcrnode = getAddress(lcrnode->lChild);
											}
											if(lockLChild(rpnode))
											{
												if(lockRChild(rnode))
												{
													node->key = rnode->key;
													#ifdef VALUE_PRESENT
													node->value = rnode->value;
													#endif
													rpnode->lChild = getAddress(rnode->rChild);
													unlockRChild(pnode);
													unlockLChild(node);
													unlockRChild(node);
													#ifdef DEBUG_ON
													printf("Success CD%lu\t%lu\n",deleteKey,getAddress(parentHead->lChild)->key);
													#endif
													return(true);
												}
												else
												{
													unlockRChild(pnode);
													unlockLChild(node);
													unlockRChild(node);
													unlockLChild(rpnode);
												}
											}
											else
											{
												unlockRChild(pnode);
												unlockLChild(node);
												unlockRChild(node);
											}
										}
										else
										{
											if(lockRChild(rnode))
											{
												node->key = rnode->key;
												#ifdef VALUE_PRESENT
												node->value = rnode->value;
												#endif
												node->rChild = getAddress(rnode->rChild);
												unlockRChild(pnode);
												unlockLChild(node);
												#ifdef DEBUG_ON
												printf("Success CD%lu\t%lu\n",deleteKey,getAddress(parentHead->lChild)->key);
												#endif
												return(true);
											}
											else
											{
												unlockRChild(pnode);
												unlockLChild(node);
												unlockRChild(node);
											}
										}
									}
								}
							}
							else
							{
								unlockRChild(pnode);
								unlockLChild(node);
							}
						}
						else
						{
							unlockRChild(pnode);
						}
					}
				}
      }
      else //if(deleteKey == node->key)
      {
        #ifdef DEBUG_ON
        printf("Failed D%lu\t%lu\n",deleteKey,getAddress(parentHead->lChild)->key);
        #endif
        return(false);
      }
    }
    else // for if(node != NULL)
    {
      #ifdef DEBUG_ON
      printf("Failed D%lu\t%lu\n",deleteKey,getAddress(parentHead->lChild)->key);
      #endif
      return(false);
    } 
  } // end of infinite while loop
}

void nodeCount(struct node* node)
{
  if(node == NULL)
  {
    return;
  }
  numOfNodes++;
  nodeCount(node->lChild);
  nodeCount(node->rChild);
}

unsigned long size()
{
  numOfNodes=0;
  nodeCount(parentHead->lChild);
  return numOfNodes;
}

void printKeysInOrder(struct node* node)
{
  if(node == NULL)
  {
    return;
  }
  printKeysInOrder(node->lChild);
  printf("%lu\t",node->key);
  printKeysInOrder(node->rChild);

}

void printKeys()
{
  printKeysInOrder(parentHead);
  printf("\n");
}

