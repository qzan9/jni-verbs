/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

package ac.ncic.syssw.azq.JniExamples;

/**
 * A simple mutable integer type.
 *
 * <p>
 * This class is used to simulate C pointer behavior.
 * <p>
 * This may be not safe for Java, so be careful when using it.
 *
 * @author azq
 */
public class MutableInteger {
	private long value;

	public MutableInteger(long value) {
		this.value = value;
	}

	public void set(long value) {
		this.value = value;
	}

	public long intValue() {
		return this.value;
	}
}
