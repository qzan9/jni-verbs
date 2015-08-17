/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

package ac.ncic.syssw.azq.JniExamples;

public class Main {
	public static void main(String[] args) {
		MutableInteger devNum = new MutableInteger(-1);
		long devList = IBVerbsNative.ibvGetDeviceList(devNum);
		System.out.println(devList);
		System.out.println(devNum.intValue());
		IBVerbsNative.ibvFreeDeviceList(devList);
	}
}
