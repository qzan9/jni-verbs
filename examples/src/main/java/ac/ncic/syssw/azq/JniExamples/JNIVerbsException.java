/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

package ac.ncic.syssw.azq.JniExamples;

public class JNIVerbsException extends Exception {
	public JNIVerbsException()
	{
	}

	public JNIVerbsException(String message)
	{
		super(message);
	}

	public JNIVerbsException(Throwable cause)
	{
		super(cause);
	}

	public JNIVerbsException(String message, Throwable cause)
	{
		super(message, cause);
	}

	public JNIVerbsException(String message, Throwable cause,
	                         boolean enableSuppression, boolean writableStackTrace)
	{
		super(message, cause, enableSuppression, writableStackTrace);
	}
}