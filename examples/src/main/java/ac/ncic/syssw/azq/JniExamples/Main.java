/*
 * Copyright (c) 2015  AZQ
 */

package ac.ncic.syssw.azq.JniExamples;

import java.nio.ByteBuffer;

import static ac.ncic.syssw.azq.JniExamples.JniRdma.*;

public class Main {
	public static void main(String[] args) {
		try {
			RdmaUserConfig userConfig = new RdmaUserConfig(65536, (args.length == 0) ? null : args[0], 9999);
			ByteBuffer buffer = rdmaInit(userConfig);
			if (userConfig.getServerName() == null) {    // server
				String data = "it's been a long day without you my friend, and i'll tell you all about it when i see you again ...";
				System.out.println("data (String): " + data);
				buffer.putInt(data.length());    // buffer: first half for write.
				buffer.put(data.getBytes());
				System.out.println("server writing to remote client buffer through RDMA ...");
				rdmaWrite();
			} else {    // client
				System.out.println("client reading local buffer ...");
				while(true) if (buffer.getInt(userConfig.getBufferSize()) > 0) break;    // buffer: second half for read.
				buffer.position(userConfig.getBufferSize());
				byte[] bytes = new byte[buffer.getInt()];
				buffer.get(bytes);
				String data = new String(bytes);
				System.out.println("data (String): " + data);
			}
		} catch (RdmaException e) {
			System.out.println("RDMA error! check your native IB/OFED/TCP configuration!");
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			rdmaFree();
		}
	}
}
