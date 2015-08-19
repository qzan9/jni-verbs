/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

package ac.ncic.syssw.azq.JniExamples;

public class JniVerbsException extends Exception {
	public JniVerbsException()
	{
	}

	public JniVerbsException(String message)
	{
		super(message);
	}

	public JniVerbsException(Throwable cause)
	{
		super(cause);
	}

	public JniVerbsException(String message, Throwable cause)
	{
		super(message, cause);
	}

	public JniVerbsException(String message, Throwable cause,
	                         boolean enableSuppression, boolean writableStackTrace)
	{
		super(message, cause, enableSuppression, writableStackTrace);
	}
}
