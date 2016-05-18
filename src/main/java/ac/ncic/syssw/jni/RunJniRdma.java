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
			int bufferSize;
			int sliceSize;
			long bufferAddress;
			byte[] data;
			long startTime, elapsedTime;

			RdmaUserConfig userConfig;

			InputStream is;
			PrintStream ps;
			Random random;
			Properties prop;
			Date date;

			ps = new PrintStream(new FileOutputStream("jni_log", true));
			date = new Date();

			System.out.println();
			System.out.println("====== RDMA write benchmarking ======");
			ps.println();
			ps.println("====== " + date.toString() + " ======");

			System.out.println();
			System.out.println("Setting up parameters ...");
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

			System.out.println();
			System.out.println("initializing RDMA ...");
			userConfig = new RdmaUserConfig(bufferSize, null, 9999);
			ByteBuffer buffer = JniRdma.rdmaInit(userConfig);
			System.out.println();
			System.out.println("RDMA buffer info:");
			System.out.println("- capacity: "    + buffer.capacity()  );
			System.out.println("- limit: "       + buffer.limit()     );
			System.out.println("- isDirect: "    + buffer.isDirect()  );
			System.out.println("- order: "       + buffer.order()     );
			System.out.println("- isReadOnly: "  + buffer.isReadOnly());

			data = new byte[bufferSize];
			bufferAddress = ((DirectBuffer) buffer).address();
			random = new Random();

			System.out.println();
			System.out.println("warming up ...");
			elapsedTime = System.currentTimeMillis();
			for (int t = 0; t < DEFAULT_WARMUP_ITER; t++) {
				startTime = System.nanoTime();
				random.nextBytes(data);
				buffer.put(data);
				JniRdma.rdmaWrite();
				U2Unsafe.copyByteArrayToDirectBuffer(data, 0, bufferAddress, bufferSize);
				JniRdma.rdmaWriteAsync(0, bufferSize);
				JniRdma.rdmaPollCq(1);
				buffer.clear();
				if (t%100 == 0) {
					System.out.printf("%d: ", t);
					System.out.println(startTime + ", getting warmer ...");
				}
			}
			System.out.printf("done! time of warming up is %d ms.", System.currentTimeMillis() - elapsedTime);
			System.out.println();

			System.out.println();
			System.out.println("benchmarking ByteBuffer.put (data re-generated each iteration) ...");
			elapsedTime = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				random.nextBytes(data);
				startTime = System.nanoTime();
				buffer.put(data);
				elapsedTime += System.nanoTime() - startTime;
				buffer.clear();
			}
			System.out.printf("average time of putting %d bytes data into DirectByteBuffer is %.1f ns.", bufferSize, (double) elapsedTime / DEFAULT_BMK_ITER);
			System.out.println();

			System.out.println();
			System.out.println("benchmarking Unsafe.copyMemory (data NOT re-generated each iteration) ...");
			random.nextBytes(data);
//			startTime = System.nanoTime();
			elapsedTime = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				startTime = System.nanoTime();
				U2Unsafe.copyByteArrayToDirectBuffer(data, 0, bufferAddress, bufferSize);
				elapsedTime += System.nanoTime() - startTime;
			}
//			elapsedTime = System.nanoTime() - startTime;
			System.out.printf("average time of unsafe.copying %d bytes data into DirectByteBuffer is %.1f ns.", bufferSize, (double) elapsedTime / DEFAULT_BMK_ITER);
			System.out.println();
			ps.printf("UNSAFE COPY %d bytes: %.1f ns.", bufferSize, (double) elapsedTime / DEFAULT_BMK_ITER);
			ps.println();

			System.out.println();
			System.out.println("benchmarking RDMA-write (data NOT re-generated each iteration) ...");
//			startTime = System.nanoTime();
			elapsedTime = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				startTime = System.nanoTime();
				JniRdma.rdmaWrite();
				elapsedTime += System.nanoTime() - startTime;
			}
//			elapsedTime = System.nanoTime() - startTime;
			System.out.printf("average time of RDMA writing %d bytes is %.1f ns.", bufferSize, (double) elapsedTime / DEFAULT_BMK_ITER);
			System.out.println();
			ps.printf("R-WRITE %d bytes: %.1f ns.", bufferSize, (double) elapsedTime / DEFAULT_BMK_ITER);
			ps.println();

			System.out.println();
			System.out.println("benchmarking x-stage pipelined RDMA-write (data NOT re-generated each iteration) ...");
//			startTime = System.nanoTime();
			elapsedTime = 0;
			for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
				startTime = System.nanoTime();
				U2Unsafe.copyByteArrayToDirectBuffer(data, 0, bufferAddress, 2048);
				JniRdma.rdmaWriteAsync(0, 2048);
				U2Unsafe.copyByteArrayToDirectBuffer(data, 2048, bufferAddress + 2048, 16384);
				JniRdma.rdmaWriteAsync(2048, 16384);
				U2Unsafe.copyByteArrayToDirectBuffer(data, 18432, bufferAddress + 18432, 32768);
				JniRdma.rdmaWriteAsync(18432, 32768);
				U2Unsafe.copyByteArrayToDirectBuffer(data, 51200, bufferAddress + 51200, 65536);
				JniRdma.rdmaWriteAsync(51200, 65536);
				U2Unsafe.copyByteArrayToDirectBuffer(data, 116736, bufferAddress + 116736, 131072);
				JniRdma.rdmaWriteAsync(116736, 131072);
				U2Unsafe.copyByteArrayToDirectBuffer(data, 247808, bufferAddress + 247808, 262144);
				JniRdma.rdmaWriteAsync(247808, 262144);
				U2Unsafe.copyByteArrayToDirectBuffer(data, 509952, bufferAddress + 509952, 14336);
				JniRdma.rdmaWriteAsync(509952, 14336);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				JniRdma.rdmaPollCq(1);
				elapsedTime += System.nanoTime() - startTime;
			}
//			elapsedTime = System.nanoTime() - startTime;
			System.out.printf("average time of x-stage pipelined RDMA writing %d bytes is %.1f ns.\n", bufferSize, (double) elapsedTime / DEFAULT_BMK_ITER);
			ps.printf("R-WRITE x-staged-pipelined: %.1f ns.\n", (double) elapsedTime / DEFAULT_BMK_ITER);

			for (sliceSize = 2048; sliceSize < bufferSize; sliceSize *= 2) {

				System.out.println();
				System.out.println("benchmarking Unsafe.copyMemory (data NOT re-generated each iteration) ...");
//				startTime = System.nanoTime();
				elapsedTime = 0;
				for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
					startTime = System.nanoTime();
					U2Unsafe.copyByteArrayToDirectBuffer(data, 0, bufferAddress, sliceSize);
					elapsedTime += System.nanoTime() - startTime;
				}
//				elapsedTime = System.nanoTime() - startTime;
				System.out.printf("average time of %dB unsafe.copying into DirectByteBuffer is %.1f ns.", sliceSize, (double) elapsedTime / DEFAULT_BMK_ITER);
				System.out.println();
				ps.printf("UNSAFE COPY %d bytes: %.1f ns.", sliceSize, (double) elapsedTime / DEFAULT_BMK_ITER);
				ps.println();

				System.out.println();
				System.out.println("benchmarking sliced RDMA-write (data NOT re-generated each iteration) ...");
//				startTime = System.nanoTime();
				elapsedTime = 0;
				for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
					startTime = System.nanoTime();
					for (int i = 0; i < bufferSize / sliceSize; i++) {
						JniRdma.rdmaWriteAsync(sliceSize * i, sliceSize);
						JniRdma.rdmaPollCq(1);
					}
					elapsedTime += System.nanoTime() - startTime;
				}
//				elapsedTime = System.nanoTime() - startTime;
				System.out.printf("average time of %dB-sliced RDMA writing %d bytes is %.1f ns.", sliceSize, bufferSize, (double) elapsedTime / DEFAULT_BMK_ITER);
				System.out.println();
				ps.printf("R-WRITE %d-bytes-sliced: %.1f ns.", sliceSize, (double) elapsedTime / DEFAULT_BMK_ITER);
				ps.println();

				System.out.println();
				System.out.println("benchmarking async sliced RDMA-write (data NOT re-generated each iteration) ...");
//				startTime = System.nanoTime();
				elapsedTime = 0;
				for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
					startTime = System.nanoTime();
					for (int i = 0; i < bufferSize / sliceSize; i++) {
						JniRdma.rdmaWriteAsync(sliceSize * i, sliceSize);
					}
					for (int i = 0; i < bufferSize / sliceSize; i++) {
						JniRdma.rdmaPollCq(1);
					}
					elapsedTime += System.nanoTime() - startTime;
				}
//				elapsedTime = System.nanoTime() - startTime;
				System.out.printf("average time of %dB-sliced async RDMA writing %d bytes is %.1f ns.", sliceSize, bufferSize, (double) elapsedTime / DEFAULT_BMK_ITER);
				System.out.println();
				ps.printf("R-WRITE %d-bytes-sliced-async: %.1f ns.", sliceSize, (double) elapsedTime / DEFAULT_BMK_ITER);
				ps.println();

				System.out.println();
				System.out.println("benchmarking fully pipelined RDMA-write (data NOT re-generated each iteration) ...");
//				startTime = System.nanoTime();
				elapsedTime = 0;
				for (int t = 0; t < DEFAULT_BMK_ITER; t++) {
					startTime = System.nanoTime();
					for (int i = 0; i < bufferSize / sliceSize; i++) {
						U2Unsafe.copyByteArrayToDirectBuffer(data, sliceSize * i, bufferAddress + sliceSize * i, sliceSize);
						JniRdma.rdmaWriteAsync(sliceSize * i, sliceSize);
					}
					for (int i = 0; i < bufferSize / sliceSize; i++) {
						JniRdma.rdmaPollCq(1);
					}
					elapsedTime += System.nanoTime() - startTime;
				}
//				elapsedTime = System.nanoTime() - startTime;
				System.out.printf("average time of %dB-sliced pipelined RDMA writing %d bytes is %.1f ns.", sliceSize, bufferSize, (double) elapsedTime / DEFAULT_BMK_ITER);
				System.out.println();
				ps.printf("R-WRITE %d-bytes-sliced-pipelined: %.1f ns.", sliceSize, (double) elapsedTime / DEFAULT_BMK_ITER);
				ps.println();
			}

			System.out.println("\ntell client to shutdown ...");
			buffer.clear();
			buffer.order(ByteOrder.nativeOrder());
			buffer.putInt(Integer.MAX_VALUE);
			JniRdma.rdmaWriteAsync(0, buffer.position());
			JniRdma.rdmaPollCq(1);

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
