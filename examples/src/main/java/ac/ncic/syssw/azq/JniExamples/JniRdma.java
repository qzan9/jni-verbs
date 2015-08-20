package ac.ncic.syssw.azq.JniExamples;

import java.nio.ByteBuffer;

public final class JniRdma {
	static {
		System.loadLibrary("jnirdma");
	}

	public static native ByteBuffer rdmaContextInit(RdmaUserConfig userConfig) throws RdmaException;
	public static native void rdmaResourceRelease();

	public static native void rdmaWrite();
	public static native void rdmaRead ();
}
