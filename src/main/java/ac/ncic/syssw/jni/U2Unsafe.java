/**
 * wrapper around sun.misc.Unsafe.
 *
 * Author(s):
 *   azq  @qzan9  anzhongqi@ncic.ac.cn
 */

package ac.ncic.syssw.jni;

import java.lang.reflect.Field;

import sun.misc.Unsafe;

public final class U2Unsafe {

	private static final Unsafe _UNSAFE;

	public static final long BYTE_ARRAY_OFFSET;
//	public static final long BYTE_ARRAY_SCALE;

	static {
		Unsafe unsafe;
		try {
			Field unsafeField = Unsafe.class.getDeclaredField("theUnsafe");
			unsafeField.setAccessible(true);
			unsafe = (Unsafe) unsafeField.get(null);
		} catch (Throwable cause) {
			unsafe = null;
		}
		_UNSAFE = unsafe;

		if (_UNSAFE != null) {
			BYTE_ARRAY_OFFSET = _UNSAFE.arrayBaseOffset(byte[].class);
//			BYTE_ARRAY_SCALE  = _UNSAFE.arrayIndexScale(byte[].class);
		} else {
			BYTE_ARRAY_OFFSET = 0;
//			BYTE_ARRAY_SCALE  = 0;
		}
	}

	public static void copyByteArray(byte[] src, long srcOffset, byte[] dest, long destOffset, long length) {
		assert srcOffset  >= 0 : "illegal source offset!";
		assert srcOffset  <= src.length  - length : "illegal source offset!";
		assert destOffset >= 0 : "illegal destination offset!";
		assert destOffset <= dest.length - length : "illegal destination offset!";
		_UNSAFE.copyMemory(src, BYTE_ARRAY_OFFSET + srcOffset, dest, BYTE_ARRAY_OFFSET + destOffset, length);
	}

	public static void throwException(Throwable t) {
		_UNSAFE.throwException(t);
	}
}
