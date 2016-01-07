/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

package ac.ncic.syssw.azq.JniExamples;

public class VerbsException extends Exception {
	public VerbsException() {
	}

	public VerbsException(String message) {
		super(message);
	}

	public VerbsException(Throwable cause) {
		super(cause);
	}

	public VerbsException(String message, Throwable cause) {
		super(message, cause);
	}

	public VerbsException(String message, Throwable cause,
	                      boolean enableSuppression, boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}
}
