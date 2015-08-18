/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

package ac.ncic.syssw.azq.JniExamples;

/**
 * A collection of native methods for accessing native RDMA verbs APIs.
 *
 * @author azq
 */
final class IBVerbsNative {
	static {
		System.loadLibrary("jniverbs");
	}

	private IBVerbsNative() {}

	/**
	 * A wrapper around <code></>ibv_get_device_list()</code>.
	 *
	 * @param devNum the device number will be written to it; it has to be an
	 *        allocated object.
	 *
	 * @return the value of <code>struct ibv_device **dev_list</code>, or the
	 *         virtual address of device list.
	 */
	static native long ibvGetDeviceList(MutableInteger devNum);

	/**
	 * Direct call into <code>ibv_free_device_list()</code>.
	 *
	 * @param devList the address value of <code>**dev_list</code>.
	 */
	static native void ibvFreeDeviceList(long devList);

	static native String ibvGetDeviceName(long devList, int devId);
	static native long ibvGetDeviceGUID(long devList, int devId);
}
