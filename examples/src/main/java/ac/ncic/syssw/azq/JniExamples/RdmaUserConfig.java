package ac.ncic.syssw.azq.JniExamples;

public class RdmaUserConfig {
	private int bufferSize;
	private int socketPort;
	private String serverName;

	public RdmaUserConfig() {
		this.bufferSize = 65536;
		this.socketPort = 9999;
		this.serverName = null;
	}

	public RdmaUserConfig(int bufferSize, int socketPort, String serverName) {
		this.bufferSize = bufferSize;
		this.socketPort = socketPort;
		this.serverName = serverName;
	}

	public void setBufferSize(int bufferSize) {
		this.bufferSize = bufferSize;
	}

	public int getBufferSize() {
		return this.bufferSize;
	}

	public void setSocketPort(int socketPort) {
		this.socketPort = socketPort;
	}

	public int getSocketPort() {
		return this.socketPort;
	}

	public void setServerName(String serverName) {
		this.serverName = serverName;
	}

	public String getServerName() {
		return this.serverName;
	}
}
