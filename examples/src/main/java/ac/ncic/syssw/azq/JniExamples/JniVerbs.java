/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

package ac.ncic.syssw.azq.JniExamples;

/**
 * A collection of native methods for accessing native IB verbs APIs.
 *
 * @author azq
 */
final class JniVerbs {
	static {
		System.loadLibrary("jniverbs");
	}

	private JniVerbs() {}

	/**
	 * A wrapper around <code>ibv_get_device_list()</code>.
	 *
	 * @param devNum to which the number of devices will be written.
	 *
	 * @return the value of <code>struct ibv_device **dev_list</code>, or the
	 *         virtual address of device list. be cautious, this is considered
	 *         unsafe in Java.
	 */
	static native long ibvGetDeviceList(MutableInteger devNum) throws VerbsException;

	/**
	 * Another wrapper around <code>ibv_get_device_list()</code>.
	 *
	 * <p>
	 * TODO Object should be a peer Java class for struct ibv_device, and how to handle the device list pointer???
	 * <p>
	 * This kind of passing back means that much more data will have to go
	 * through JNI, which I'd like to avoid.
	 */
	static native Object[] ibvGetDeviceList() throws VerbsException;

	/**
	 * Direct call into <code>ibv_free_device_list()</code>.
	 *
	 * @param devListAddr the address value of <code>**dev_list</code>.
	 */
	static native void ibvFreeDeviceList(long devListAddr);

	/**
	 * Invoke <code>ibv_get_device_name</code> and return.
	 *
	 * @param devListAddr the pointer/address of device list array.
	 *
	 * @param devId the device id (0, 1, 2, ...).
	 *
	 * @return the name of the device as a Java String.
	 */
	static native String ibvGetDeviceName(long devListAddr, int devId);

	/**
	 * @see #ibvGetDeviceName(long, int)
	 */
	static native long ibvGetDeviceGUID(long devListAddr, int devId);
}
