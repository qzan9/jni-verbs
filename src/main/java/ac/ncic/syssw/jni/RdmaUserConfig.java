package ac.ncic.syssw.jni;

public class RdmaUserConfig {
	private int bufferSize;
	private String serverName;
	private int socketPort;

	public RdmaUserConfig() {
		this.bufferSize = 65536;
		this.serverName = null;
		this.socketPort = 9999;
	}

	public RdmaUserConfig(int bufferSize, String serverName, int socketPort) {
		this.bufferSize = bufferSize;
		this.serverName = serverName;
		this.socketPort = socketPort;
	}

	public void setBufferSize(int bufferSize) {
		this.bufferSize = bufferSize;
	}

	public int getBufferSize() {
		return this.bufferSize;
	}

	public void setServerName(String serverName) {
		this.serverName = serverName;
	}

	public String getServerName() {
		return this.serverName;
	}

	public void setSocketPort(int socketPort) {
		this.socketPort = socketPort;
	}

	public int getSocketPort() {
		return this.socketPort;
	}
}
