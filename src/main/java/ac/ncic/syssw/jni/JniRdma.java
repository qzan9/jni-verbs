/*
 * Copyright (c) 2015  AZQ
 */

package ac.ncic.syssw.jni;

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
	 * Perform asynchronous RDMA write operation.
	 */
	public static native void rdmaWriteAsync(int offset, int length) throws RdmaException;

	/**
	 * Poll CQ entries.
	 */
	public static native void rdmaPollCq(int numEntries) throws RdmaException;

	/**
	 * Free/destroy RDMA resources.
	 */
	public static native void rdmaFree();
}
