package ac.ncic.syssw.azq.JniExamples;

public class RdmaException extends Exception {
	public RdmaException() {
	}

	public RdmaException(String message) {
		super(message);
	}

	public RdmaException(Throwable cause) {
		super(cause);
	}

	public RdmaException(String message, Throwable cause) {
		super(message, cause);
	}

	public RdmaException(String message, Throwable cause,
	                     boolean enableSuppression, boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}
}
