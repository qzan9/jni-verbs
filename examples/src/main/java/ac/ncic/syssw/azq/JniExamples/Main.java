/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

package ac.ncic.syssw.azq.JniExamples;

import static ac.ncic.syssw.azq.JniExamples.JniVerbs.*;

public class Main {
	public static void main(String[] args) {
		long devListAddr = -1;
		try {
			MutableInteger devNum = new MutableInteger(-1);
			devListAddr = ibvGetDeviceList(devNum);
			System.out.printf("virtual address of device list: %#x.\n", devListAddr);
			System.out.printf("%d devices found.\n", devNum.intValue());
			for (int i = 0; i < devNum.intValue(); i++)
				System.out.printf("%d: %s %#x\n", i, ibvGetDeviceName(devListAddr, i), ibvGetDeviceGUID(devListAddr, i));
		} catch (VerbsException e) {
			System.out.println(e.getMessage());
			System.out.println("check your IB/OFED configuration!");
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (devListAddr != -1)    // this is not safe for Java.
				ibvFreeDeviceList(devListAddr);
		}
	}
}
