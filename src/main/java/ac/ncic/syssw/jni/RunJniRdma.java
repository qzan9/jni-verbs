/**
 * Exercise the JniRdma native methods and collect micro-benchmarking info.
 *
 * Author(s):
 *   azq  @qzan9  anzhongqi@ncic.ac.cn
 */

package ac.ncic.syssw.jni;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import java.util.Random;
import java.util.Properties;
import java.util.Date;

import sun.nio.ch.DirectBuffer;

public class RunJniRdma {

	public static final int DEFAULT_WARMUP_ITER = 16000;
	public static final int DEFAULT_BMK_ITER    = 1000;

	public static final int DEFAULT_BUFFER_SIZE = 524288;
	public static final int DEFAULT_SLICE_SIZE  = 131072;

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

			int bufferSize;
			int sliceSize;
			long bufferAddress;
			byte[] data;
			long elapsedTimeSum;

			InputStream is;
			PrintStream ps;
			Random random;
			Properties prop;
			Date date;

			ps = new PrintStream(new FileOutputStream("jni_log", true));
			date = new Date();

			System.out.println("\n====== RDMA write benchmarking ======");
			ps.println("\n====== " + date.toString() + " ======");

			System.out.println("\nSetting up parameters ...");
			try {
				is = new FileInputStream("jni_config");
				prop = new Properties();
				prop.load(is);
				is.close();

				if (prop.getProperty("bufferSize") != null) {
					bufferSize = Integer.parseInt(prop.getProperty("bufferSize"));
				} else {
					bufferSize = DEFAULT_BUFFER_SIZE;
				}

				if (prop.getProperty("sliceSize") != null) {
					sliceSize = Integer.parseInt(prop.getProperty("sliceSize"));
				} else {
					sliceSize = DEFAULT_SLICE_SIZE;
				}

			} catch (FileNotFoundException e) {
				System.out.println("jni_config file not found! using DEFAULT configurations!");
				bufferSize = DEFAULT_BUFFER_SIZE;
				sliceSize  = DEFAULT_SLICE_SIZE;
			}/* catch (Exception e) {
				e.printStackTrace();
			} finally {
			}*/
			System.out.println("Benchmarking configurations:");
			System.out.println("- buffer size: " + bufferSize);
			System.out.println("- slice size: "  + sliceSize);
			ps.println("buffer size: " + bufferSize + ", slice size: " + sliceSize);

			System.out.println("\ninitializing RDMA ...");
			userConfig = new RdmaUserConfig(bufferSize, null, 9999);
			ByteBuffer buffer = JniRdma.rdmaInit(userConfig);
			System.out.println("RDMA buffer info:");
			System.out.println("- capacity: "    + buffer.capacity()  );
			System.out.println("- limit: "       + buffer.limit()     );
			System.out.println("- isDirect: "    + buffer.isDirect()  );
			System.out.println("- order: "       + buffer.order()     );
			System.out.println("- isReadOnly: "  + buffer.isReadOnly());

			data = new byte[bufferSize];
			bufferAddress = ((DirectBuffer) buffer).address();
			random = new Random();

			System.out.println("\nwarming up ...");
			elapsedTimeSum = System.currentTimeMillis();
			for (int t = 0; t < DEFAULT_WARMUP_ITER; t++) {
				long startTime = System.nanoTime();
				random.nextBytes(data);
				buffer.put(data);
				JniRdma.rdmaWrite();
				startTime = System.nanoTime();
				U2Unsafe.copyByteArrayToDirectBuffer(data, 0, bufferAddress, bufferSize);
				JniRdma.rdmaWriteAsync(0, bufferSize);
				JniRdma.rdmaPollCq(1);
				buffer.clear();
				if (t%100 == 0) {
					System.out.printf("#%d: ", t);
					System.out.println(startTime + ", getting warmer ...");
				}
			}
			System.out.printf("done! elapsed time is %d ms.\n", System.currentTimeMillis() - elapsedTimeSum);

			System.out.println("\nbenchmarking ByteBuffer.put (data re-generated each iteration) ...");
			elapsedTimeSum = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				random.nextBytes(data);

				long startTime = System.nanoTime();
				buffer.put(data);
				elapsedTimeSum += System.nanoTime() - startTime;

				buffer.clear();
			}
			System.out.printf("average time of putting %d bytes data into DirectByteBuffer is %.1f ns.\n", bufferSize, (double) elapsedTimeSum / DEFAULT_BMK_ITER);

			System.out.println("\nbenchmarking Unsafe.copyMemory (data NOT re-generated each iteration) ...");
			random.nextBytes(data);
			elapsedTimeSum = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				long startTime  = System.nanoTime();
				U2Unsafe.copyByteArrayToDirectBuffer(data, 0, bufferAddress, sliceSize);
				elapsedTimeSum += System.nanoTime() - startTime;
			}
			System.out.printf("average time of unsafe.copying %d bytes data into DirectByteBuffer is %.1f ns.\n", sliceSize, (double) elapsedTimeSum / DEFAULT_BMK_ITER);
			ps.printf("UNSAFE COPY %d bytes: %.1f ns.\n", sliceSize, (double) elapsedTimeSum / DEFAULT_BMK_ITER);
			elapsedTimeSum = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				long startTime  = System.nanoTime();
				U2Unsafe.copyByteArrayToDirectBuffer(data, 0, bufferAddress, bufferSize);
				elapsedTimeSum += System.nanoTime() - startTime;
			}
			System.out.printf("average time of unsafe.copying %d bytes data into DirectByteBuffer is %.1f ns.\n", bufferSize, (double) elapsedTimeSum / DEFAULT_BMK_ITER);
			ps.printf("UNSAFE COPY %d bytes: %.1f ns.\n", bufferSize, (double) elapsedTimeSum / DEFAULT_BMK_ITER);

			System.out.println("\nbenchmarking RDMA-write (data NOT re-generated each iteration) ...");
			elapsedTimeSum = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				long startTime  = System.nanoTime();
				JniRdma.rdmaWrite();
				elapsedTimeSum += System.nanoTime() - startTime;
			}
			System.out.printf("average time of RDMA writing %d bytes is %.1f ns.\n", bufferSize, (double) elapsedTimeSum / DEFAULT_BMK_ITER);
			ps.printf("R-WRITE: %.1f ns.\n", (double) elapsedTimeSum / DEFAULT_BMK_ITER);

			System.out.println("\nbenchmarking sliced RDMA-write (data NOT re-generated each iteration) ...");
			elapsedTimeSum = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				long startTime  = System.nanoTime();
				for (int i = 0; i < bufferSize / sliceSize; i++) {
					JniRdma.rdmaWriteAsync(sliceSize * i, sliceSize);
					JniRdma.rdmaPollCq(1);
				}
				elapsedTimeSum += System.nanoTime() - startTime;
			}
			System.out.printf("average time of sliced RDMA writing %d bytes is %.1f ns.\n", bufferSize, (double) elapsedTimeSum / DEFAULT_BMK_ITER);
			ps.printf("sliced R-WRITE: %.1f ns.\n", (double) elapsedTimeSum / DEFAULT_BMK_ITER);

			System.out.println("\nbenchmarking async sliced RDMA-write (data NOT re-generated each iteration) ...");
			elapsedTimeSum = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				long startTime  = System.nanoTime();
				for (int i = 0; i < bufferSize / sliceSize; i++) {
					JniRdma.rdmaWriteAsync(sliceSize * i, sliceSize);
				}
				for (int i = 0; i < bufferSize / sliceSize; i++) {
					JniRdma.rdmaPollCq(1);
				}
				elapsedTimeSum += System.nanoTime() - startTime;
			}
			System.out.printf("average time of async sliced RDMA writing %d bytes is %.1f ns.\n", bufferSize, (double) elapsedTimeSum / DEFAULT_BMK_ITER);
			ps.printf("async sliced R-WRITE: %.1f ns.\n", (double) elapsedTimeSum / DEFAULT_BMK_ITER);

			System.out.println("\nbenchmarking 2-stage pipelined RDMA-write (data NOT re-generated each iteration) ...");
			elapsedTimeSum = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				long startTime  = System.nanoTime();
				U2Unsafe.copyByteArrayToDirectBuffer(data, 0, bufferAddress, sliceSize);
				JniRdma.rdmaWriteAsync(0, sliceSize);
				U2Unsafe.copyByteArrayToDirectBuffer(data, sliceSize, bufferAddress + sliceSize, bufferSize - sliceSize);
				JniRdma.rdmaWriteAsync(sliceSize, bufferSize - sliceSize);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				elapsedTimeSum += System.nanoTime() - startTime;
			}
			System.out.printf("average time of 2-stage pipelined RDMA writing %d bytes is %.1f ns.\n", bufferSize, (double) elapsedTimeSum / DEFAULT_BMK_ITER);
			ps.printf("2-staged PIPELINING: %.1f ns.\n", (double) elapsedTimeSum / DEFAULT_BMK_ITER);

			System.out.println("\nbenchmarking x-stage pipelined RDMA-write (data NOT re-generated each iteration) ...");
			elapsedTimeSum = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				long startTime = System.nanoTime();
				U2Unsafe.copyByteArrayToDirectBuffer(data,      0, bufferAddress,            2048);
				JniRdma.rdmaWriteAsync(0,        2048);
				U2Unsafe.copyByteArrayToDirectBuffer(data,   2048, bufferAddress +  2048,   16384);
				JniRdma.rdmaWriteAsync(2048,    16384);
				U2Unsafe.copyByteArrayToDirectBuffer(data,  18432, bufferAddress +  18432,  32768);
				JniRdma.rdmaWriteAsync(18432,   32768);
				U2Unsafe.copyByteArrayToDirectBuffer(data,  51200, bufferAddress +  51200,  65536);
				JniRdma.rdmaWriteAsync(51200,   65536);
				U2Unsafe.copyByteArrayToDirectBuffer(data, 116736, bufferAddress + 116736, 131072);
				JniRdma.rdmaWriteAsync(116736, 131072);
				U2Unsafe.copyByteArrayToDirectBuffer(data, 247808, bufferAddress + 247808, 262144);
				JniRdma.rdmaWriteAsync(247808, 262144);
				U2Unsafe.copyByteArrayToDirectBuffer(data, 509952, bufferAddress + 509952,  14336);
				JniRdma.rdmaWriteAsync(509952,  14336);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				elapsedTimeSum += System.nanoTime() - startTime;
			}
			System.out.printf("average time of x-stage pipelined RDMA writing %d bytes is %.1f ns.\n", bufferSize, (double) elapsedTimeSum / DEFAULT_BMK_ITER);
			ps.printf("x-staged PIPELINING: %.1f ns.\n", (double) elapsedTimeSum / DEFAULT_BMK_ITER);

			if (bufferSize % sliceSize == 0) {
				System.out.println("\nbenchmarking fully pipelined RDMA-write (data NOT re-generated each iteration) ...");
				elapsedTimeSum = 0;
				for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
					long startTime = System.nanoTime();
					for (int i = 0; i < bufferSize / sliceSize; i++) {
						U2Unsafe.copyByteArrayToDirectBuffer(data, sliceSize * i, bufferAddress + sliceSize * i, sliceSize);
						JniRdma.rdmaWriteAsync(sliceSize * i, sliceSize);
					}
					for (int i = 0; i < bufferSize / sliceSize; i++) {
						JniRdma.rdmaPollCq(1);
					}
					elapsedTimeSum += System.nanoTime() - startTime;
				}
				System.out.printf("average time of fully pipelined RDMA writing %d bytes is %.1f ns.\n", bufferSize, (double) elapsedTimeSum / DEFAULT_BMK_ITER);
				ps.printf("full PIPELINING: %.1f ns.\n", (double) elapsedTimeSum / DEFAULT_BMK_ITER);

				System.out.println("\ntell client to shutdown ...");
				buffer.clear();
				buffer.order(ByteOrder.nativeOrder());
				buffer.putInt(Integer.MAX_VALUE);
				JniRdma.rdmaWriteAsync(0, buffer.position());
				JniRdma.rdmaPollCq(1);
			}

			ps.println();
			ps.close();

		} catch (RdmaException e) {
			System.out.println("RDMA error! check your native IB/OFED/TCP configuration!");
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			JniRdma.rdmaFree();
		}
	}
}
