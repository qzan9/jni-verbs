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
//				System.out.println("server writing to remote client buffer through RDMA ...");
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
			RdmaUserConfig userConfig = new RdmaUserConfig(512, null, 9999);
			ByteBuffer buffer = JniRdma.rdmaInit(userConfig);

			System.out.println("=== RDMA write benchmarking ===");

			System.out.println("warming up ...");
			Random random = new Random();
			byte[] data = new byte[512];
			for (int i = 0; i < 100; i++) {
				random.nextBytes(data);
			}

			long elapsedTimeSum = 0;
			for (int t = 0; t < 1000; t++) {
				random.nextBytes(data);

				long startTime = System.nanoTime();
				buffer.put(data);
				elapsedTimeSum += System.nanoTime() - startTime;
			}
			System.out.printf("average time of putting %d KB data into DirectByteBuffer is %d us us.\n", 512, elapsedTimeSum/1000000);

//			JniRdma.rdmaWrite();
			JniRdma.rdmaWriteAsync(0, data.length);
			JniRdma.rdmaPollCq(1);
		} catch (RdmaException e) {
			System.out.println("RDMA error! check your native IB/OFED/TCP configuration!");
		} catch (Exception e) {

		} finally {
			JniRdma.rdmaFree();
		}
	}
}
