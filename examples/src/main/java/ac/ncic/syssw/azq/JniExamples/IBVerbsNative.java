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

	static native long ibvGetDeviceList(MutableInteger devNum);
	static native void ibvFreeDeviceList(long devList);
}
