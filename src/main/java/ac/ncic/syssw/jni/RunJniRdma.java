/*
 * Exercise the JniRdma native methods and collect microbenchmarking info.
 *
 * Author(s):
 *   azq  @qzan9  anzhongqi@ncic.ac.cn
 */

package ac.ncic.syssw.jni;

import java.nio.ByteBuffer;
import java.util.Random;

public class RunJniRdma {
	private RunJniRdma() { }

	private static RunJniRdma INSTANCE;
	public  static RunJniRdma getInstance() {
		if (null == INSTANCE) {
			INSTANCE = new RunJniRdma();
		}
		return INSTANCE;
	}

	public void simpleTest(String[] args) {
		try {
			RdmaUserConfig userConfig = new RdmaUserConfig(65536, (args.length == 0) ? null : args[0], 9999);
			ByteBuffer buffer = JniRdma.rdmaInit(userConfig);

			if (userConfig.getServerName() == null) {    // server
				String data = "it's been a long day without you my friend, and i'll tell you all about it when i see you again ...";
				System.out.println("data (String): " + data);
				buffer.putInt(data.length());
				buffer.put(data.getBytes());
				System.out.println("server writing to remote client buffer through RDMA ...");
				JniRdma.rdmaWrite();
			} else {    // client
				System.out.println("client reading local buffer ...");
//				while(true) if (buffer.getInt(userConfig.getBufferSize()) > 0) break;
				while(true) if (buffer.getInt(0) > 0) break;
//				buffer.position(userConfig.getBufferSize());
				byte[] bytes = new byte[buffer.getInt()];
				buffer.get(bytes);
				String data = new String(bytes);
				System.out.println("data (String): " + data);
			}
		} catch (RdmaException e) {
			System.out.println("RDMA error! check your native IB/OFED/TCP configuration!");
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			JniRdma.rdmaFree();
		}
	}

	public void simplePipeTest() {
		try {
			Random random = new Random();
			long elapsedTimeSum;

			System.out.println("====== RDMA write benchmarking ======");
			System.out.println();

			System.out.println("initializing RDMA ...");
			RdmaUserConfig userConfig = new RdmaUserConfig(524288, null, 9999);
			ByteBuffer buffer = JniRdma.rdmaInit(userConfig);
			System.out.println("RDMA buffer info:");
			System.out.println("- capacity: "    + buffer.capacity()  );
			System.out.println("- limit: "       + buffer.limit()     );
			System.out.println("- isDirect: "    + buffer.isDirect()  );
			System.out.println("- order: "       + buffer.order()     );
			System.out.println("- isReadOnly: "  + buffer.isReadOnly());
			System.out.println();

			System.out.println("warming up ...");
			byte[] data = new byte[buffer.limit()];
			for (int t = 0; t < 1000; t++) {
				random.nextBytes(data);
				buffer.put(data);
				buffer.clear();
			}
			System.out.println("done!");
			System.out.println();

			System.out.println("start measuring ByteBuffer.put() ...");
			elapsedTimeSum = 0;
			for (int t = 0; t < 10000; t++) {
				random.nextBytes(data);

				long startTime  = System.nanoTime();
				buffer.put(data);
				elapsedTimeSum += System.nanoTime() - startTime;

				buffer.clear();
			}
			System.out.printf("average time of putting %d bytes data into DirectByteBuffer is %.1f ns.\n\n", data.length, (double) elapsedTimeSum/1000);

			System.out.println("start measuring rdmaWrite() ...");
			elapsedTimeSum = 0;
			for (int t = 0; t < 10000; t++) {
				random.nextBytes(data);
				buffer.put(data);

				long startTime  = System.nanoTime();
				JniRdma.rdmaWrite();    // native method, no need to warm up.
				elapsedTimeSum += System.nanoTime() - startTime;

				buffer.clear();
			}
			System.out.printf("average time of RDMA writing %d bytes is %.1f ns.\n\n", data.length, (double) elapsedTimeSum/1000);

			System.out.println("tell client to shutdown ...");
			buffer.putInt(Integer.MAX_VALUE);
			JniRdma.rdmaWriteAsync(0, buffer.position());
			JniRdma.rdmaPollCq(1);
			buffer.clear();

		} catch (RdmaException e) {
			System.out.println("RDMA error! check your native IB/OFED/TCP configuration!");
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			JniRdma.rdmaFree();
		}
	}
}
