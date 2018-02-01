package com.rokid.speech;

public class PrepareOptions {
	public String host;
	public int port;
	public String branch;
	public String key;
	public String device_type_id;
	public String secret;
	public String device_id;
	public int reconn_interval;

	public PrepareOptions() {
		port = 0;
		reconn_interval = 20000;
	}
}
