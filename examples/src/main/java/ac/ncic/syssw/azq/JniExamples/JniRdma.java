/*
 * Copyright (c) 2015  AZQ
 */

package ac.ncic.syssw.azq.JniExamples;

import java.nio.ByteBuffer;

import static ac.ncic.syssw.azq.JniExamples.JniRdma.*;

public class Main {
	public static void main(String[] args) {
		try {
			RdmaUserConfig userConfig = new RdmaUserConfig(65536, (args.length == 0) ? null : args[0], 9999);
			ByteBuffer buffer = rdmaInit(userConfig);
			if (userConfig.getServerName() == null) {    // server
				String data = "it's been a long day without you my friend, and i'll tell you all about it when i see you again ...";
				System.out.println("data (String): " + data);
				buffer.putInt(data.length());    // buffer: first half for write.
				buffer.put(data.getBytes());
				System.out.println("server writing to remote client buffer through RDMA ...");
				rdmaWrite();
			} else {    // client
				System.out.println("client reading local buffer ...");
				while(true) if (buffer.getInt(userConfig.getBufferSize()) > 0) break;    // buffer: second half for read.
				buffer.position(userConfig.getBufferSize());
				byte[] bytes = new byte[buffer.getInt()];
				buffer.get(bytes);
				String data = new String(bytes);
				System.out.println("data (String): " + data);
			}
		} catch (RdmaException e) {
			System.out.println("RDMA error! check your native IB/OFED/TCP configuration!");
		} catch (Exception e) {
package ac.ncic.syssw.azq.JniExamples;

import java.nio.ByteBuffer;

public final class JniRdma {
	static {
		System.loadLibrary("jnirdma");
	}

	/**
	 * Get IB ready to send/receive and return a RDMA memory registered
	 * DirectByteBuffer.
	 *
	 * <p>
	 * This native method first parses user configuration, then initializes
	 * IB context (PD, MR, CQ, QP, etc.) and exchanges RDMA connection
	 * information using native sockets.
	 *
	 * <p>
	 * After this native method is done, IB is ready to performance RDMA
	 * operations and a DirectByteBuffer is returned.
	 *
	 * <p>
	 * This is still early demo, and is just for peer-to-peer test. For a
	 * buffer of size <code>bufferSize</code>, a native buffer of size
	 * <code>bufferSize*2</code> is actually allocated, with the first half
	 * for write and second half for read.
	 *
	 * @param userConfig user configuration data.
	 *
	 * @return an MR DirectByteBuffer.
	 */
	public static native ByteBuffer rdmaInit(RdmaUserConfig userConfig) throws RdmaException;

	/**
	 * Perform RDMA write operation.
	 */
	public static native void rdmaWrite() throws RdmaException;

	/**
	 * Free/destroy RDMA resources.
	 */
	public static native void rdmaFree();
}
