/*
 * Copyright (c) 2015  AZQ
 */

package ac.ncic.syssw.jni;

import java.nio.ByteBuffer;

public class Main {
	public static void main(String[] args) {
		RunJniRdma.getInstance().simplePipeTest();
	}
}
