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

	public final void lock(Node node)
	{
		while(!node.isLocked.compareAndSet(false, true))
		{
		}
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
					return(1);
				}
			}
			//System.out.println("key not found");
			return(0);
		}
	}

	public final void insert(Node root, long insertKey, int threadId)
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
						break;
					}
				}
				if(!ltLastKey)
				{
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

			lock(pnode);
			//check if parent still pointing to leaf
			if(nthChild > -1 && pnode.childrenArray[nthChild] == node && !pnode.isMarked) //and parent is unmarked
			{
				if(node.keys == null) //special node is reached
				{
					//This special node can be replaced with a new leaf node containing the key
					long[] keys = new long[Node.NUM_OF_KEYS_IN_A_NODE];
					for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
					{
						keys[i]=0;
					}
					keys[0] = insertKey;
					pnode.childrenArray[nthChild] = new Node(keys,"leafNode");
					//unlock parent
					unlock(pnode);
					return;
				}
				else //leaf node is reached
				{
					for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
					{
						if(node.keys[i] > 0)
						{
							if(insertKey == node.keys[i])
							{
								//key is already found
								//out.println(threadId  + " " + insertKey + " is already found");
								//unlock parent
								unlock(pnode);
								return;
							}
						}
						else // leaf has a empty slot
						{
							isLeafFull=false;        
							emptySlotId = i;
						}
					}

					if(!isLeafFull)
					{
						//out.println(threadId  + "Non Full Leaf Node - Trying simple insert for " + insertKey);
						node.keys[emptySlotId] = insertKey;
						//unlock parent
						unlock(pnode);
						return;
					}
					else
					{
						//here the leaf is full
						//find the minimum key in the leaf and if insert key is greater than min key then do a swap
						long[] tempInternalkeys = new long[Node.NUM_OF_KEYS_IN_A_NODE];
						for(int i =0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
						{
							tempInternalkeys[i] = node.keys[i];
						}

						//out.println(threadId  + "Trying to insert " + insertKey + " and this node" + node + " is getting replaced" + tempInternalkeys[0] + tempInternalkeys[1] + tempInternalkeys[2]);
						long extrakey;
						long min = tempInternalkeys[0];
						int  minPos = 0;
						for(int i=1;i<tempInternalkeys.length;i++)
						{
							if(tempInternalkeys[i]<min)
							{
								min = tempInternalkeys[i];
								minPos = i;
							}
						}
						if(insertKey > min)
						{
							extrakey = min;
							tempInternalkeys[minPos] = insertKey;
						}
						else
						{
							extrakey = insertKey;
						}
						Arrays.sort(tempInternalkeys);
						//out.println("by" + tempInternalkeys + " " + tempInternalkeys[0] + tempInternalkeys[1] + tempInternalkeys[2]);
						Node replaceNode = new Node(tempInternalkeys,"internalNode");
						long[] tempLeafKeys = new long[Node.NUM_OF_KEYS_IN_A_NODE];
						for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
						{
							tempLeafKeys[i]=0;
						}
						tempLeafKeys[0] = extrakey;
						replaceNode.childrenArray[0] = new Node(tempLeafKeys,"leafNode");
						for(int i=1;i<Node.NUM_OF_CHILDREN_FOR_A_NODE;i++)
						{
							tempLeafKeys[0] = tempInternalkeys[i-1];
							replaceNode.childrenArray[i] = new Node(tempLeafKeys,"leafNode");
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

			lock(gpnode);
			lock(pnode);
			//check if parent still pointing to leaf & gp still pointing to p
			if(nthChild > -1  && nthParent > -1 && gpnode.childrenArray[nthParent] == pnode && pnode.childrenArray[nthChild] == node && !pnode.isMarked && !gpnode.isMarked) //and parent is unmarked
			{
				if(node.keys == null) //special node is reached. Delete key is not present
				{
					//unlock gp & p
					unlock(gpnode);
					unlock(pnode);
					return;
				}
				else //leaf node is reached
				{
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
					if(keyFound)
					{
						if(atleast2Keys > 1) //simple delete
						{
							node.keys[keyIndex] = 0;
							unlock(gpnode);
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
								unlock(gpnode);
								unlock(pnode);
								return;
							}
							else //pruning delete. Only this node and another sibling exist. Make the gp point to the sibling.
							{
								//mark the parent node
								pnode.isMarked = true;
								for(int i=0;i<Node.NUM_OF_CHILDREN_FOR_A_NODE;i++)
								{
									if(pnode.childrenArray[i].keys != null && pnode.childrenArray[i] != node) //find the sibling
									{
										//out.println(threadId + "replacing " + pnode + " " + gpnode.childrenArray[nthParent] + " with " + pnode.childrenArray[i]);
										//out.println(threadId + " " + "gp= " + gpnode + " p= " + pnode + " l= " + node );
										gpnode.childrenArray[nthParent] = pnode.childrenArray[i];
										
										unlock(gpnode);
										unlock(pnode);
										//out.println(threadId + " did a pruning delete for " + deleteKey + " and gp= " + gpnode + " l= " + gpnode.childrenArray[nthParent]);
										return;
									}
								}
							}
						}
					}
					else
					{
						//unlock gp & p
						unlock(gpnode);
						unlock(pnode);
						return;
					}
				}
			}
			else
			{
				//unlock gp & p
				unlock(gpnode);
				unlock(pnode);
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
				System.out.print(node.keys[i] + "\t");
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

	public final void nodeCount(Node node)
	{
		if(node == null || node.equals(Node.SPL_NODE))
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
		long[] keys = new long[Node.NUM_OF_KEYS_IN_A_NODE];

		for(int i=0;i<Node.NUM_OF_KEYS_IN_A_NODE;i++)
		{
			keys[i] = Long.MAX_VALUE;
		}
		new Node(false); //create a special node

		grandParentHead = new Node(keys,"internalNode");
		grandParentHead.childrenArray[0] = new Node(keys,"internalNode");
		parentHead = grandParentHead.childrenArray[0];
	}

}
