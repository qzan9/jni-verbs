/**
 * Exercise the JniRdma native methods and collect microbenchmarking info.
 *
 * Author(s):
 *   azq  @qzan9  anzhongqi@ncic.ac.cn
 */

package ac.ncic.syssw.jni;

import java.io.InputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import java.util.Random;
import java.util.Properties;

import sun.nio.ch.DirectBuffer;

public class RunJniRdma {

	public static final int DEFAULT_BUFFER_SIZE = 524288;
	public static final int DEFAULT_PIPE_SIZE   = 147456;
	public static final int DEFAULT_WARMUP_ITER = 15000;
	public static final int DEFAULT_BMK_ITER    = 1000;

	private RunJniRdma() { }

	private static RunJniRdma INSTANCE;
	public  static RunJniRdma getInstance() {
		if (null == INSTANCE) {
			INSTANCE = new RunJniRdma();
		}
		return INSTANCE;
	}

	public void simpleTestJniVerbs() {
		long devListAddr = -1;
		try {
			MutableInteger devNum = new MutableInteger(-1);
			devListAddr = JniVerbs.ibvGetDeviceList(devNum);
			System.out.printf("virtual address of device list: %#x.\n", devListAddr);
			System.out.printf("%d devices found.\n", devNum.intValue());
			for (int i = 0; i < devNum.intValue(); i++)
				System.out.printf("%d: %s %#x\n", i, JniVerbs.ibvGetDeviceName(devListAddr, i), JniVerbs.ibvGetDeviceGUID(devListAddr, i));
		} catch (VerbsException e) {
			System.out.println("check your IB/OFED configuration!");
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (devListAddr != -1)    // this is not safe for Java.
				JniVerbs.ibvFreeDeviceList(devListAddr);
		}
	}

	public void simpleTestJniRdma(String[] args) {
		try {
			RdmaUserConfig userConfig = new RdmaUserConfig(DEFAULT_BUFFER_SIZE, (args.length == 0) ? null : args[0], 9999);
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

	public void pipelineBenchmarkJniRdma() {
		try {
			RdmaUserConfig userConfig;
			long elapsedTimeSum;
			long bufferAddress;
			byte[] data;
			int pipeSize;

			InputStream is;
			Properties prop;
			Random random;

			System.out.println("\n====== RDMA write benchmarking ======");

			System.out.println("\ninitializing RDMA ...");
			try {
				is = new FileInputStream("jni_config");
				prop = new Properties();
				prop.load(is);
				is.close();

				if (prop.getProperty("bufferSize") != null) {
					userConfig = new RdmaUserConfig(Integer.parseInt(prop.getProperty("bufferSize")), null, 9999);
				} else {
					userConfig = new RdmaUserConfig(DEFAULT_BUFFER_SIZE, null, 9999);
				}

				if (prop.getProperty("pipeSize") != null) {
					pipeSize = Integer.parseInt(prop.getProperty("pipeSize"));
				} else {
					pipeSize = userConfig.getBufferSize() / 4;
				}
			} catch (FileNotFoundException e) {
				System.out.println("jni_config file not found! using DEFAULT configurations!");
				userConfig = new RdmaUserConfig(DEFAULT_BUFFER_SIZE, null, 9999);
				pipeSize = DEFAULT_PIPE_SIZE;
			} finally {
				random = new Random();
			}

			ByteBuffer buffer = JniRdma.rdmaInit(userConfig);
			System.out.println("\nRDMA buffer info:");
			System.out.println("- capacity: "    + buffer.capacity()  );
			System.out.println("- limit: "       + buffer.limit()     );
			System.out.println("- isDirect: "    + buffer.isDirect()  );
			System.out.println("- order: "       + buffer.order()     );
			System.out.println("- isReadOnly: "  + buffer.isReadOnly());
			System.out.println("Other params:");
			System.out.println("- pipeSize: " + pipeSize);

			data = new byte[buffer.limit()];
			bufferAddress = ((DirectBuffer) buffer).address();

			System.out.println("\nwarming up ...");
			elapsedTimeSum = System.currentTimeMillis();
			for (int t = 0; t < DEFAULT_WARMUP_ITER; t++) {
				random.nextBytes(data);
				buffer.put(data);
				JniRdma.rdmaWrite();
				buffer.clear();
				long startTime = System.nanoTime();
				random.nextBytes(data);
				U2Unsafe.copyByteArrayToDirectBuffer(data, 0, bufferAddress, data.length);
				JniRdma.rdmaWriteAsync(0, data.length);
				JniRdma.rdmaPollCq(1);
				buffer.clear();
			}
			System.out.printf("done! elapsed time is %.1f ms.\n", (double)(System.currentTimeMillis()-elapsedTimeSum));

			System.out.println("\nstart measuring ByteBuffer.put() ...");
			elapsedTimeSum = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				random.nextBytes(data);

				long startTime  = System.nanoTime();
				buffer.put(data);
				elapsedTimeSum += System.nanoTime() - startTime;

				buffer.clear();
			}
			System.out.printf("average time of putting %d bytes data into DirectByteBuffer is %.1f ns.\n", data.length, (double)(elapsedTimeSum/DEFAULT_BMK_ITER));

			System.out.println("\nstart measuring Unsafe.copyMemory() ...");
			elapsedTimeSum = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				random.nextBytes(data);

				long startTime  = System.nanoTime();
				U2Unsafe.copyByteArrayToDirectBuffer(data, 0, bufferAddress, data.length);
				elapsedTimeSum += System.nanoTime() - startTime;

				buffer.clear();
			}
			System.out.printf("average time of copying %d bytes data into DirectByteBuffer is %.1f ns.\n", data.length, (double)(elapsedTimeSum/DEFAULT_BMK_ITER));

			System.out.println("\nstart measuring rdmaWrite() ...");
			elapsedTimeSum = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				random.nextBytes(data);
				buffer.put(data);

				long startTime  = System.nanoTime();
				JniRdma.rdmaWrite();
				elapsedTimeSum += System.nanoTime() - startTime;

				buffer.clear();
			}
			System.out.printf("average time of RDMA writing %d bytes is %.1f ns.\n", data.length, (double)(elapsedTimeSum/DEFAULT_BMK_ITER));

			System.out.println("\nstart measuring pipelined rdmaWrite() ...");
			elapsedTimeSum = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				random.nextBytes(data);

				long startTime  = System.nanoTime();
				U2Unsafe.copyByteArrayToDirectBuffer(data, 0, bufferAddress, pipeSize);
//				buffer.put(data, 0, pipeSize);
				JniRdma.rdmaWriteAsync(0, pipeSize);
				U2Unsafe.copyByteArrayToDirectBuffer(data, pipeSize, bufferAddress, data.length-pipeSize);
//				buffer.put(data, pipeSize, data.length-pipeSize);
				JniRdma.rdmaWriteAsync(pipeSize, data.length-pipeSize);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				elapsedTimeSum += System.nanoTime() - startTime;

				buffer.clear();
			}
			System.out.printf("average time of pipelined RDMA writing %d bytes is %.1f ns.\n", data.length, (double)elapsedTimeSum/ DEFAULT_BMK_ITER);

			System.out.println("\ntell client to shutdown ...");
			buffer.order(ByteOrder.nativeOrder());
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
