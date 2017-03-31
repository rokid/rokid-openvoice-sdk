package com.rokid.speech;

import java.util.HashMap;
import java.io.FileInputStream;
import java.io.IOException;
import android.util.Log;
import com.alibaba.fastjson.JSON;

public class Tts {
	public Tts(String configFile) {
		callbacks_ = new HashMap<Long, TtsCallback>();
		_sdk_tts = _sdk_create();

		config_tts(configFile);
	}

	public void finalize() {
		_sdk_release(_sdk_tts);
		_sdk_delete(_sdk_tts);
	}

	public boolean prepare() {
		return _sdk_prepare(_sdk_tts);
	}

	public int speak(String content, TtsCallback cb) {
		long sdk_cb = _sdk_create_callback(cb);
		addCallback(sdk_cb, cb);
		int id = _sdk_speak(content, sdk_cb, _sdk_tts);
		Log.d(TAG, "speak " + content + ", id = " + id);
		if (id <= 0)
			eraseCallback(sdk_cb);
		return id;
	}

	public void stop(int id) {
		_sdk_stop(id, _sdk_tts);
	}

	public void config(String key, String value) {
		_sdk_config(key, value, _sdk_tts);
	}

	private void addCallback(long sdk_cb, TtsCallback cb) {
		synchronized (callbacks_) {
			callbacks_.put(sdk_cb, cb);
		}
	}

	// invoked by native '_sdk_cb' when '_sdk_cb' release
	private void eraseCallback(long sdk_cb) {
		synchronized (callbacks_) {
			callbacks_.remove(sdk_cb);
		}
	}

	private void config_tts(String configFile) {
		FileInputStream is = null;
		byte[] content;

		try {
			is = new FileInputStream(configFile);
			int size = is.available();
			Log.d(TAG, "config file size = " + size);
			content = new byte[size];
			is.read(content);
			Log.d(TAG, "config file content = " + new String(content));
		} catch (Exception e) {
			e.printStackTrace();
			return;
		} finally {
			if (is != null) {
				try {
					is.close();
				} catch (IOException e) {
				}
			}
		}
		TtsConfig config = JSON.parseObject(content, TtsConfig.class);
		Log.d(TAG, "config file parse result = " + config);
		_sdk_config("server_address", config.server_address, _sdk_tts);
		_sdk_config("ssl_roots_pem", config.ssl_roots_pem, _sdk_tts);
		_sdk_config("auth_key", config.auth_key, _sdk_tts);
		_sdk_config("device_type", config.device_type, _sdk_tts);
		_sdk_config("device_id", config.device_id, _sdk_tts);
		_sdk_config("secret", config.secret, _sdk_tts);
		_sdk_config("api_version", config.api_version, _sdk_tts);
	}

	private native long _sdk_create();

	private native void _sdk_delete(long sdk_tts);

	private native boolean _sdk_prepare(long sdk_tts);

	private native void _sdk_release(long sdk_tts);

	private native long _sdk_create_callback(TtsCallback cb);

	private native int _sdk_speak(String content, long sdk_cb, long sdk_tts);

	private native void _sdk_stop(int id, long sdk_tts);

	private native void _sdk_config(String key, String value, long sdk_tts);

	private HashMap<Long, TtsCallback> callbacks_;

	static {
		System.loadLibrary("rokid_tts_jni");
	}

	private static final String TAG = "tts.sdk";

	private long _sdk_tts;
}

class TtsConfig {
	public String server_address;

	public String ssl_roots_pem;

	public String auth_key;

	public String device_type;

	public String device_id;

	public String secret;

	public String api_version;

	public String toString() {
		return "server_address=" + server_address
			+ ", ssl_roots_pem=" + ssl_roots_pem
			+ ", auth_key=" + auth_key
			+ ", device_type=" + device_type
			+ ", device_id=" + device_id
			+ ", secret=" + secret
			+ ", api_version=" + api_version;
	}
}
