import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.PrintStream;
import java.util.Arrays;
public class LockBasedKaryST
{

	static Node grandParentHead;
	static Node parentHead;
	static LockBasedKaryST obj;
	static long nodeCount=0;
	static FileOutputStream outf;
	static PrintStream out;
	public LockBasedKaryST()
	{
		try 
		{
			outf = new FileOutputStream("out.txt");
			out = new PrintStream(outf);
		}
		catch (FileNotFoundException e)
		{
			e.printStackTrace();
		}
	}

	public final boolean lock(Node node)
	{
		while(!node.isLocked.compareAndSet(false, true))
		{
			if(node.isMarked)
			{
				return false;
			}
		}
		return true;
	}

	public final void unlock(Node node)
	{
		node.isLocked.set(false);     
	}

	public final long lookup(Node node, long target)
	{
		boolean ltLastKey;
		while(node.childrenArray !=null) //loop until a leaf or special node is reached
		{
			ltLastKey=false;
			for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
			{
				if(target < node.keys[i])
				{
					ltLastKey = true;
					node = node.childrenArray[i];
					break;
				}
			}
			if(!ltLastKey)
			{
				node = node.childrenArray[Node.NUM_OF_KEYS_IN_A_NODE];
			}
		}
		if(node.keys == null) //special node is reached
		{
			//System.out.println("key not found");
			return(0);
		}
		else //leaf node is reached
		{
			for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
			{
				if(target == node.keys[i])
				{
					//System.out.println("key found");
					return(node.values[i]);
				}
			}
			//System.out.println("key not found");
			return(0);
		}
	}

	public final void insert(Node root, long insertValue, long insertKey, int threadId)
	{
		boolean ltLastKey;
		boolean isLeafFull;
		int emptySlotId;
		int nthChild;
		Node node;
		Node pnode;
		while(true)
		{
			ltLastKey=false;
			isLeafFull=true;
			emptySlotId=0;
			nthChild=-1;
			node = root;
			pnode = null;

			while(node.childrenArray !=null) //loop until a leaf or special node is reached
			{
				ltLastKey=false;

				for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
				{
					if(insertKey < node.keys[i])
					{
						ltLastKey = true;
						pnode = node;
						node = node.childrenArray[i];
						nthChild = i;
						break;
					}
				}
				if(!ltLastKey)
				{
					pnode = node;
					node = node.childrenArray[Node.NUM_OF_KEYS_IN_A_NODE];
					nthChild = Node.NUM_OF_KEYS_IN_A_NODE;
				}
			}

//			for(int i =0;i<Node.NUM_OF_CHILDREN_FOR_A_NODE;i++) //get the child id w.r.t the parent
//			{
//				if(pnode.childrenArray[i] == node)
//				{
//					nthChild = i;
//					break;
//				}
//			}
			if(node.keys != null)
			{
				for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
				{
					if(node.keys[i] > 0)
					{
						if(insertKey == node.keys[i])
						{
							//key is already found
							return;
						}
					}
					else // leaf has a empty slot
					{
						isLeafFull=false;        
						emptySlotId = i;
					}
				}
			}

			if(lock(pnode))
			{
				//check if parent still pointing to leaf
				if(nthChild > -1 && pnode.childrenArray[nthChild] == node && !pnode.isMarked) //and parent is unmarked
				{
					if(node.keys == null) //special node is reached
					{
						//This special node can be replaced with a new leaf node containing the key
						long[] keys   = new long[Node.NUM_OF_KEYS_IN_A_NODE];
						long[] values = new long[Node.NUM_OF_KEYS_IN_A_NODE];
						for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
						{
							keys[i]=0;
							values[i]=0;
						}
						keys[0] = insertKey;
						values[0] = insertValue;
						pnode.childrenArray[nthChild] = new Node(keys, values,"leafNode");
						//unlock parent
						unlock(pnode);
						return;
					}
					else //leaf node is reached
					{
						if(!isLeafFull)
						{
							//out.println(threadId  + "Non Full Leaf Node - Trying simple insert for " + insertKey);
							node.keys[emptySlotId] = insertKey;
							node.values[emptySlotId] = insertValue;
							//unlock parent
							unlock(pnode);
							return;
						}
						else
						{
							//here the leaf is full
							//find the minimum key in the leaf and if insert key is greater than min key then do a swap
							long[] tempInternalkeys = new long[Node.NUM_OF_KEYS_IN_A_NODE];
							long[] tempInternalvalues = new long[Node.NUM_OF_KEYS_IN_A_NODE];
							for(int i =0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
							{
								tempInternalkeys[i] = node.keys[i];
								tempInternalvalues[i] = node.values[i];
							}

							//out.println(threadId  + "Trying to insert " + insertKey + " and this node" + node + " is getting replaced" + tempInternalkeys[0] + tempInternalkeys[1] + tempInternalkeys[2]);
							long extrakey;
							long extravalue;
							long min = tempInternalkeys[0];
							long minvalue = tempInternalvalues[0];
							int  minPos = 0;
							for(int i=1;i<tempInternalkeys.length;i++)
							{
								if(tempInternalkeys[i]<min)
								{
									min = tempInternalkeys[i];
									minvalue=tempInternalvalues[i];
									minPos = i;
								}
							}
							if(insertKey > min)
							{
								extrakey = min;
								extravalue = minvalue;
								tempInternalkeys[minPos] = insertKey;
								tempInternalvalues[minPos] = insertValue;
							}
							else
							{
								extrakey = insertKey;
								extravalue = insertValue;
							}
							for(int i=1;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
							{
								long tempk = tempInternalkeys[i];
								long tempv = tempInternalvalues[i];
								int j=i-1;
								while(j>=0 && tempInternalkeys[j] > tempk)
								{
									tempInternalkeys[j+1] = tempInternalkeys[j];
									tempInternalvalues[j+1] = tempInternalvalues[j];
									j--;
								}
								tempInternalkeys[j+1] = tempk;
								tempInternalvalues[j+1] = tempv;
							}
							//out.println("by" + tempInternalkeys + " " + tempInternalkeys[0] + tempInternalkeys[1] + tempInternalkeys[2]);
							Node replaceNode = new Node(tempInternalkeys, tempInternalvalues, "internalNode");
							long[] tempLeafKeys = new long[Node.NUM_OF_KEYS_IN_A_NODE];
							long[] tempLeafvalues = new long[Node.NUM_OF_KEYS_IN_A_NODE];
							for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
							{
								tempLeafKeys[i]=0;
								tempLeafvalues[i]=0;
							}
							tempLeafKeys[0] = extrakey;
							tempLeafvalues[0] = extravalue;
							replaceNode.childrenArray[0] = new Node(tempLeafKeys, tempLeafvalues, "leafNode");
							for(int i=1;i<Node.NUM_OF_CHILDREN_FOR_A_NODE;i++)
							{
								tempLeafKeys[0] = tempInternalkeys[i-1];
								tempLeafvalues[0] = tempInternalvalues[i-1];
								replaceNode.childrenArray[i] = new Node(tempLeafKeys, tempLeafvalues, "leafNode");
							}
							pnode.childrenArray[nthChild] = replaceNode;
							//unlock parent
							unlock(pnode);
							return;
						}
					}
				}
				else
				{
					//unlock parent
					unlock(pnode);
				}
			}
		}
	}

	public final void delete(Node proot, Node gproot, long deleteKey, int threadId)
	{
		boolean ltLastKey;
		boolean keyFound;
		int keyIndex;
		int nthChild;
		int nthParent;
		int atleast2Keys;
		Node node;
		Node pnode;
		Node gpnode;
		while(true)
		{
			ltLastKey=false;
			keyFound=false;
			keyIndex=-1;
			nthChild=-1;
			nthParent=-1;
			atleast2Keys=0;
			pnode=proot;
			gpnode=gproot;
			node=proot.childrenArray[0];

			while(node.childrenArray !=null) //loop until a leaf or special node is reached
			{
				ltLastKey=false;

				for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
				{
					if(deleteKey < node.keys[i])
					{
						ltLastKey = true;
						gpnode = pnode;
						pnode = node;
						node = node.childrenArray[i];
						break;
					}
				}
				if(!ltLastKey)
				{
					gpnode=pnode;
					pnode = node;
					node = node.childrenArray[Node.NUM_OF_KEYS_IN_A_NODE];
				}
			}

			for(int i =0;i<Node.NUM_OF_CHILDREN_FOR_A_NODE;i++) //get the child id w.r.t the parent
			{
				if(pnode.childrenArray[i] == node)
				{
					nthChild = i;
					break;
				}
			}
			for(int i =0;i<Node.NUM_OF_CHILDREN_FOR_A_NODE;i++) //get the parent id w.r.t the grandparent
			{
				if(gpnode.childrenArray[i] == pnode)
				{
					nthParent = i;
					break;
				}
			}

			if(node.keys == null) //special node is reached. Delete key is not present
			{
				return;
			}
			for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
			{
				if(node.keys[i] > 0)
				{
					atleast2Keys++;
				}
				if(deleteKey == node.keys[i])
				{

					keyFound=true;
					keyIndex=i;
				}
			}
			if(!keyFound)
			{
				return;
			}
			if(lock(pnode))
			{
				//check if parent still pointing to leaf & gp still pointing to p
				if(nthChild > -1  && nthParent > -1 && gpnode.childrenArray[nthParent] == pnode && pnode.childrenArray[nthChild] == node && !pnode.isMarked) //and parent is unmarked
				{
					//leaf node is reached

					if(atleast2Keys > 1) //simple delete
					{
						node.keys[keyIndex] = 0;
						node.values[keyIndex] = 0;
						unlock(pnode);
						return;
					}
					else //only 1 key is present in leaf. Have to check if parent has at least 3 non-special children.
					{
						int nonDummyChildCount=0;
						for(int i=0;i<Node.NUM_OF_CHILDREN_FOR_A_NODE;i++)
						{
							if(pnode.childrenArray[i].keys != null)
							{
								nonDummyChildCount++;
							}
						}
						if(nonDummyChildCount != 2) //simple delete. Replace leaf node with a special node
						{
							//pnode.childrenArray[nthChild] = new SpecialNode();
							pnode.childrenArray[nthChild] = Node.SPL_NODE;
							unlock(pnode);
							return;
						}
						else //pruning delete. Only this node and another sibling exist. Make the gp point to the sibling.
						{
							//mark the parent node
							if(lock(gpnode))
							{
								if(!gpnode.isMarked && gpnode.childrenArray[nthParent] == pnode)
								{
									pnode.isMarked = true;
									for(int i=0;i<Node.NUM_OF_CHILDREN_FOR_A_NODE;i++)
									{
										if(pnode.childrenArray[i].keys != null && pnode.childrenArray[i] != node) //find the sibling
										{
											//out.println(threadId + "replacing " + pnode + " " + gpnode.childrenArray[nthParent] + " with " + pnode.childrenArray[i]);
											//out.println(threadId + " " + "gp= " + gpnode + " p= " + pnode + " l= " + node );
											gpnode.childrenArray[nthParent] = pnode.childrenArray[i];

											unlock(pnode);
											unlock(gpnode);
											//out.println(threadId + " did a pruning delete for " + deleteKey + " and gp= " + gpnode + " l= " + gpnode.childrenArray[nthParent]);
											return;
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
					//unlock gp & p
					unlock(pnode);
				}
			}
		}
	}

	public final void printPreorder(Node node)
	{
		if(node == null)
		{
			return;
		}
		if(node.keys != null)
		{
			for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
			{
				System.out.print(node.keys[i] + "\t");
			}
		}
		else
		{
			System.out.print("special Node");
		}
		System.out.println();
		if(node.childrenArray != null)
		{
			for(int i=0;i<Node.NUM_OF_CHILDREN_FOR_A_NODE;i++)
			{
				printPreorder(node.childrenArray[i]);
			}
		}
	}

	public final void printOnlyKeysPreorder(Node node)	
	{
		if(node == null || node.equals(Node.SPL_NODE))
		{
			return;
		}
		if(node.childrenArray == null && node.keys != null)
		{
			for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
			{
				System.out.print(node.keys[i] + "," + node.values[i] + "\t");
			}
			System.out.println();
		}

		if(node.childrenArray != null)
		{
			for(int i=0;i<Node.NUM_OF_CHILDREN_FOR_A_NODE;i++)
			{
				printOnlyKeysPreorder(node.childrenArray[i]);
			}
		}
	}
	
	public final void printOnlyKeysInorder(Node node)	
	{
		if(node == null || node.equals(Node.SPL_NODE))
		{
			return;
		}
		if(node.childrenArray == null && node.keys != null)
		{
			for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
			{
				//System.out.print(node.keys[i] + "," + node.values[i] + "\t"); //with values
				System.out.print(node.keys[i] + "\t");
			}
			System.out.println();
		}

		if(node.childrenArray != null)
		{
			for(int i=0;i<Node.NUM_OF_CHILDREN_FOR_A_NODE;i++)
			{
				printOnlyKeysInorder(node.childrenArray[i]);
			}
		}
	}
	

	public final void nodeCount(Node node)
	{
		if(node == null || node.keys ==null)
		//if(node == null || node.equals(Node.SPL_NODE))
		{
			return;
		}
		if(node.childrenArray == null && node.keys != null)
		{
			for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
			{
				if(node.keys[i] > 0)
				{
					nodeCount++;
				}
			}
		}

		if(node.childrenArray != null)
		{
			for(int i=0;i<Node.NUM_OF_CHILDREN_FOR_A_NODE;i++)
			{
				nodeCount(node.childrenArray[i]);
			}
		}
	}

	public final void createHeadNodes()
	{
		long[] keys   = new long[Node.NUM_OF_KEYS_IN_A_NODE];
		long[] values = new long[Node.NUM_OF_KEYS_IN_A_NODE];

		for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
		{
			keys[i] = Long.MAX_VALUE;
			values[i] = Long.MAX_VALUE;
		}
		new Node(false); //create a special node

		grandParentHead = new Node(keys,values,"internalNode");
		grandParentHead.childrenArray[0] = new Node(keys,values,"internalNode");
		parentHead = grandParentHead.childrenArray[0];
	}

}
