/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

package ac.ncic.syssw.azq.JniExamples;

import static ac.ncic.syssw.azq.JniExamples.IBVerbsNative.*;

public class Main {
	public static void main(String[] args) {
		try {
			MutableInteger devNum = new MutableInteger(-1);
			long devList = ibvGetDeviceList(devNum);
			System.out.printf("virtual address of device list: %#x.\n", devList);
			System.out.printf("%d devices found.\n", devNum.intValue());
			for (int i = 0; i < devNum.intValue(); i++)
				System.out.printf("%d: %s %#x\n", i, ibvGetDeviceName(devList, i), ibvGetDeviceGUID(devList, i));
			IBVerbsNative.ibvFreeDeviceList(devList);
		} catch (Exception e) {
			System.out.println(e.getMessage());
		}
	}
}
