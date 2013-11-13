import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.util.StringTokenizer;


public class TestLockBasedKaryST extends LockBasedKaryST implements Runnable
{
	int threadId;
	public static int NUM_OF_THREADS;

	public TestLockBasedKaryST(int threadId)
	{
		this.threadId = threadId;
	}

	final void getUserInput(int fileNumber)
	{
		String in="";
		String operation="";
		StringTokenizer st;
		FileInputStream fs;
		try
		{
			fs = new FileInputStream("../input/in" + fileNumber + ".txt");
			DataInputStream ds = new DataInputStream(fs);
			BufferedReader reader = new BufferedReader(new InputStreamReader(ds));
			while(!(in = reader.readLine()).equalsIgnoreCase("quit"))
			{
				st = new StringTokenizer(in);
				operation = st.nextToken();
				if(operation.equalsIgnoreCase("Find"))
				{
					obj.lookup(grandParentHead,Long.parseLong(st.nextToken()));
				}
				else if(operation.equalsIgnoreCase("Insert"))
				{
					obj.insert(grandParentHead,Long.parseLong(st.nextToken()),threadId);
				}
				else if(operation.equalsIgnoreCase("Delete"))
				{
					obj.delete(parentHead,grandParentHead,Long.parseLong(st.nextToken()),threadId);
				}

			}
			ds.close();
		} 
		catch(Exception e)
		{
			e.printStackTrace();
		}

	}

	public void run()
	{
		getUserInput(this.threadId);
	}

	public static void main(String[] args)
	{
		NUM_OF_THREADS = Integer.parseInt(args[0]);
		try
		{
			obj = new LockBasedKaryST();
			obj.createHeadNodes();

			Thread[] arrayOfThreads = new Thread[NUM_OF_THREADS+1];

			arrayOfThreads[0] = new Thread(  new TestLockBasedKaryST(0)); //just inserts - initial array
			arrayOfThreads[0].start();
			arrayOfThreads[0].join();
			//System.out.println("Thread " + 0 + " is done");


			for(int i=1;i<=NUM_OF_THREADS;i++)
			{
				arrayOfThreads[i] = new Thread(  new TestLockBasedKaryST(i));
				arrayOfThreads[i].start();
			}


			for(int i=1;i<=NUM_OF_THREADS;i++)
			{
				arrayOfThreads[i].join();
				//System.out.println("Thread " + i + " is done");
			}
			//obj.printPreorder(LockBasedKaryST.grandParentHead);
			//obj.printOnlyKeysPreorder(LockBasedKaryST.grandParentHead);
			obj.nodeCount(LockBasedKaryST.grandParentHead);
			System.out.println(LockBasedKaryST.nodeCount);
		}
		catch (InterruptedException e) 
		{
			e.printStackTrace();
		}

	}
}
