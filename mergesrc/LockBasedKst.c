#include"header.h"

struct node
{
  unsigned long keys[NUM_OF_KEYS_IN_A_NODE];
  unsigned long values[NUM_OF_KEYS_IN_A_NODE];
  struct node** childrenArray;
  tbb::atomic<bool> isLocked;
  bool isMarked;
};

struct node* grandParentHead=NULL;
struct node* parentHead=NULL;
struct node* SPL_NODE=NULL;
unsigned long numOfNodes;

void newSpecialNode()
{
  SPL_NODE = (struct node*) malloc(sizeof(struct node));
  for(int i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
  {
    SPL_NODE->keys[i] = 0;
    SPL_NODE->values[i] =0;
  }
  SPL_NODE->childrenArray = NULL;
  SPL_NODE->isLocked = false;
  SPL_NODE->isMarked = false;
}

struct node* newLeafNode(unsigned long keys[], unsigned long values[])
{
  struct node* node = (struct node*) malloc(sizeof(struct node));
  for(int i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
  {
    node->keys[i] = keys[i];
    node->values[i] = values[i];
  }
  node->childrenArray = NULL;
  node->isLocked = false;
  node->isMarked = false;
  return(node);
}

struct node* newInternalNode(unsigned long keys[], unsigned long values[])
{
  int i;
  struct node* node = (struct node*) malloc(sizeof(struct node));
  for(i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
  {
    node->keys[i] = keys[i];
    node->values[i] = values[i];
  }
  node->childrenArray = (struct node**) malloc(NUM_OF_CHILDREN_FOR_A_NODE * sizeof(struct node*));
  for(i=0;i<NUM_OF_CHILDREN_FOR_A_NODE;i++)
  {
    node->childrenArray[i] = SPL_NODE;
  }
  node->isLocked = false;
  node->isMarked = false;

  return(node);
}

void createHeadNodes()
{
  int i;
  unsigned long keys[NUM_OF_KEYS_IN_A_NODE];
  unsigned long values[NUM_OF_KEYS_IN_A_NODE];
  newSpecialNode();
  for(i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
  {
    keys[i] = ULONG_MAX;
    values[i] = ULONG_MAX;
  }
  grandParentHead = newInternalNode(keys,values);
  grandParentHead->childrenArray[0] = newInternalNode(keys,values);
  parentHead = grandParentHead->childrenArray[0];
}

bool lock(struct node* node)
{
  while(node->isLocked.compare_and_swap(true,false))
  {
    if(node->isMarked)
    {
      return false;
    }
  }
  return true;
}

void unlock(struct node* node)
{
  node->isLocked = false;
}

unsigned long lookup(unsigned long target)
{
  int i;
  bool ltLastKey;
  struct node* node = grandParentHead;

  while(node->childrenArray != NULL) //loop until a leaf or speacial node is reached
  {
    ltLastKey = false;
    for(i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
    {
      if(target < node->keys[i])
      {
        ltLastKey = true;
        node = node->childrenArray[i];
        break;
      }
    }
    if(!ltLastKey)
    {
      node = node->childrenArray[NUM_OF_KEYS_IN_A_NODE];
    }
  }
  if(node == SPL_NODE) //speacial node is reached
  {
    return(0);
  }
  else //leaf node is reached
  {
    for(i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
    {
      if(target == node->keys[i])
      {
        return(node->values[i]);
      }
    }
    return(0);
  }
}

bool insert(unsigned long insertKey, unsigned long insertValue)
{
  int i,j;
  bool ltLastKey;
  bool isLeafFull;
  int emptySlotId;
  int nthChild;
  struct node* node;
  struct node* pnode;

  while(true)
  {
    ltLastKey = false;
    isLeafFull = true;
    emptySlotId = INT_MAX;
    nthChild = -1;
    node = grandParentHead;
    pnode = NULL;

    while(node->childrenArray != NULL) //loop until a leaf or special node is reached
    {
      ltLastKey = false;
      for(i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
      {
        if(insertKey < node->keys[i])
        {
          ltLastKey = true;
          pnode = node;
          node = node->childrenArray[i];
          nthChild = i;
          break;
        }
      }
      if(!ltLastKey)
      {
        pnode = node;
        node = node->childrenArray[NUM_OF_KEYS_IN_A_NODE];
        nthChild = NUM_OF_KEYS_IN_A_NODE;
      }
    }

    if(node->keys != NULL)
    {
      for(i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
      {
        if(node->keys[i] > 0 )
        {
          if(insertKey == node->keys[i])
          {
            //key is already found
            return false;
          }
        }
        else //leaf has a empty slot
        {
          isLeafFull = false;
          if(i<emptySlotId)
          {
            emptySlotId = i;
          }
        }
      }
    }

    if(lock(pnode))
    {
      //check if parent is still pointing to leaf
      if(pnode->childrenArray[nthChild] == node && !pnode->isMarked) //and parent is not marked
      {
        if(node == SPL_NODE)
        {
          //this special node can be replaced with new leaf node containing the insert key
          unsigned long keys[NUM_OF_KEYS_IN_A_NODE];
          unsigned long values[NUM_OF_KEYS_IN_A_NODE];
          for(i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
          {
            keys[i] = 0;
            values[i] = 0;
          }
          keys[0] = insertKey;
          values[0] = insertValue;
          pnode->childrenArray[nthChild] = newLeafNode(keys, values);
          unlock(pnode);
          return true;
        }
        else //leaf node is reached
        {
          if(!isLeafFull)
          {
            node->keys[emptySlotId] = insertKey;
            node->values[emptySlotId] = insertValue;
            unlock(pnode);
            return true;
          }
          else //leaf is full
          {
            //find the minimum key in the leaf and if insert key is greater than min key then do a swap
            unsigned long tempInternalKeys[NUM_OF_KEYS_IN_A_NODE];
            unsigned long tempInternalValues[NUM_OF_KEYS_IN_A_NODE];
            for(i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
            {
              tempInternalKeys[i] = node->keys[i];
              tempInternalValues[i] = node->values[i];
            }
            unsigned long extraKey;
            unsigned long extraValue;
            unsigned long min = tempInternalKeys[0];
            unsigned long minValue = tempInternalValues[0];
            int minPos = 0;
            for(i=1;i<NUM_OF_KEYS_IN_A_NODE;i++)
            {
              if(tempInternalKeys[i] < min)
              {
                min = tempInternalKeys[i];
                minValue = tempInternalValues[i];
                minPos = i;
              }
            }
            if(insertKey > min)
            {
              extraKey = min;
              extraValue = minValue;
              tempInternalKeys[minPos] = insertKey;
              tempInternalValues[minPos] = insertValue;
            }
            else
            {
              extraKey = insertKey;
              extraValue = insertValue;
            }
            for(i=1;i<NUM_OF_KEYS_IN_A_NODE;i++)
            {
              unsigned long tempK = tempInternalKeys[i];
              unsigned long tempV = tempInternalValues[i];
              j = i-1;
              while(j>=0 && tempInternalKeys[j] > tempK)
              {
                tempInternalKeys[j+1] = tempInternalKeys[j];
                tempInternalValues[j+1] = tempInternalValues[j];
                j--;
              }
              tempInternalKeys[j+1] = tempK;
              tempInternalValues[j+1] = tempV;
            }
            struct node* replaceNode = newInternalNode(tempInternalKeys, tempInternalValues);
            unsigned long tempLeafKeys[NUM_OF_KEYS_IN_A_NODE];
            unsigned long tempLeafValues[NUM_OF_KEYS_IN_A_NODE];
            for(i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
            {
              tempLeafKeys[i] = 0;
              tempLeafValues[i] = 0;
            }
            tempLeafKeys[0] = extraKey;
            tempLeafValues[0] = extraValue;
            replaceNode->childrenArray[0] = newLeafNode(tempLeafKeys, tempLeafValues);
            for(i=1;i<NUM_OF_CHILDREN_FOR_A_NODE;i++)
            {
              tempLeafKeys[0] = tempInternalKeys[i-1];
              tempLeafValues[0] = tempInternalValues[i-1];
              replaceNode->childrenArray[i] = newLeafNode(tempLeafKeys, tempLeafValues);
            }
            free(pnode->childrenArray[nthChild]);
            pnode->childrenArray[nthChild] = replaceNode;
            unlock(pnode);
            return true;
          }
        }
      }
      else
      {
        unlock(pnode);
      }
    }
  }
}

bool remove(unsigned long deleteKey)
{
	bool ltLastKey;
	bool keyFound;
	int keyIndex;
	int nthChild;
	int nthParent;
	int atleast2Keys;
	struct node* node;
	struct node* pnode;
	struct node* gpnode;
	while(true)
	{
		ltLastKey=false;
		keyFound=false;
		keyIndex=-1;
		nthChild=-1;
		nthParent=-1;
		atleast2Keys=0;
		pnode=parentHead;
		gpnode=grandParentHead;
		node=pnode->childrenArray[0];

		while(node->childrenArray != NULL) //loop until a leaf or special node is reached
		{
			ltLastKey=false;

			for(int i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
			{
				if(deleteKey < node->keys[i])
				{
					ltLastKey = true;
					gpnode = pnode;
					pnode = node;
					node = node->childrenArray[i];
					break;
				}
			}
			if(!ltLastKey)
			{
				gpnode=pnode;
				pnode = node;
				node = node->childrenArray[NUM_OF_KEYS_IN_A_NODE];
			}
		}

		if(node == SPL_NODE) //special node is reached. Delete key is not present
		{
			return false;
		}
		
		for(int i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
		{
			if(node->keys[i] > 0)
			{
				atleast2Keys++;
			}
			if(deleteKey == node->keys[i])
			{

				keyFound=true;
				keyIndex=i;
			}
		}
		
		if(!keyFound)
		{
			return false;
		}
		
		for(int i=0;i<NUM_OF_CHILDREN_FOR_A_NODE;i++) //get the child id w.r.t the parent
		{
			if(pnode->childrenArray[i] == node)
			{
				nthChild = i;
				break;
			}
		}
		for(int i =0;i<NUM_OF_CHILDREN_FOR_A_NODE;i++) //get the parent id w.r.t the grandparent
		{
			if(gpnode->childrenArray[i] == pnode)
			{
				nthParent = i;
				break;
			}
		}	
		
		if(lock(pnode))
		{
			//check if parent still pointing to leaf & gp still pointing to p
			if(gpnode->childrenArray[nthParent] == pnode && pnode->childrenArray[nthChild] == node && !pnode->isMarked) //and parent is unmarked
			{
				//leaf node is reached

				if(atleast2Keys > 1) //simple delete
				{
					node->keys[keyIndex] = 0;
					node->values[keyIndex] = 0;
					unlock(pnode);
					return true;
				}
				else //only 1 key is present in leaf. Have to check if parent has at least 3 non-special children.
				{
					int nonDummyChildCount=0;
					for(int i=0;i<NUM_OF_CHILDREN_FOR_A_NODE;i++)
					{
						if(pnode->childrenArray[i] != SPL_NODE)
						{
							nonDummyChildCount++;
						}
					}
					if(nonDummyChildCount != 2) //simple delete. Replace leaf node with a special node
					{
						free(pnode->childrenArray[nthChild]);
            pnode->childrenArray[nthChild] = SPL_NODE;
						unlock(pnode);
						return true;
					}
					else //pruning delete. Only this node and another sibling exist. Make the gp point to the sibling.
					{
						//mark the parent node
						if(lock(gpnode))
						{
							if(!gpnode->isMarked && gpnode->childrenArray[nthParent] == pnode)
							{
								pnode->isMarked = true;
								for(int i=0;i<NUM_OF_CHILDREN_FOR_A_NODE;i++)
								{
									if(pnode->childrenArray[i] != SPL_NODE && pnode->childrenArray[i] != node) //find the sibling
									{
										gpnode->childrenArray[nthParent] = pnode->childrenArray[i]; 
 										unlock(pnode);
										unlock(gpnode);
                    free(node);
                    free(pnode);
										return true;
									}
								}
							}
							else
							{
								unlock(pnode);
								unlock(gpnode);
							}
						}
						else
						{
							unlock(pnode);
						}
					}
				}
			}
			else
			{
				unlock(pnode);
			}
		}
	}
}

void nodeCount(struct node* node)
{
  if(node == NULL || node == SPL_NODE)
  {
    return;
  }
  if(node->childrenArray == NULL)
  {
    for(int i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
    {
      if(node->keys[i] > 0)
      {
        numOfNodes++;
      }
    }
  }
  else
  {
    for(int i=0;i<NUM_OF_CHILDREN_FOR_A_NODE;i++)
    {
       nodeCount(node->childrenArray[i]);
    }
  }
}

unsigned long size()
{
  numOfNodes=0;
  nodeCount(parentHead);
  return numOfNodes;
}

void printOnlyKeysInOrder(struct node* node)
{
  if(node == NULL || node == SPL_NODE)
  {
    return;
  }

  if(node->childrenArray == NULL)
  {
    for(int i=0;i<NUM_OF_KEYS_IN_A_NODE;i++)
    {
      if(node->keys[i] > 0)
      {
        printf("%lu\t",node->keys[i]);
      }
    }
    printf("\n");
  }
  else
  {
    for(int i=0;i<NUM_OF_CHILDREN_FOR_A_NODE;i++)
    {
       printOnlyKeysInOrder(node->childrenArray[i]);
    }
  }
}

void printKeys()
{
  printOnlyKeysInOrder(parentHead);
}
