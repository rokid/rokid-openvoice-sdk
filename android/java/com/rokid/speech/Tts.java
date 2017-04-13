package com.rokid.speech;

import java.io.FileInputStream;
import java.io.IOException;
import android.util.Log;
import android.util.SparseArray;
import com.alibaba.fastjson.JSON;

public class Tts {
	public Tts(String configFile) {
		_callbacks = new SparseArray<TtsCallback>();
		_sdk_tts = _sdk_create();
		config_tts(configFile);
		_sdk_prepare(_sdk_tts);
	}

	public void finalize() {
		_sdk_release(_sdk_tts);
		_sdk_delete(_sdk_tts);
	}

	public int speak(String content, TtsCallback cb) {
		int id;
		synchronized (_callbacks) {
			id = _sdk_speak(_sdk_tts, content);
			Log.d(TAG, "speak " + content + ", id = " + id);
			if (id > 0)
				_callbacks.put(id, cb);
		}
		return id;
	}

	public void cancel(int id) {
		_sdk_cancel(_sdk_tts, id);
	}

	public void config(String key, String value) {
		_sdk_config(_sdk_tts, key, value);
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
		_sdk_config(_sdk_tts, "server_address", config.server_address);
		_sdk_config(_sdk_tts, "ssl_roots_pem", config.ssl_roots_pem);
		_sdk_config(_sdk_tts, "auth_key", config.auth_key);
		_sdk_config(_sdk_tts, "device_type_id", config.device_type_id);
		_sdk_config(_sdk_tts, "device_id", config.device_id);
		_sdk_config(_sdk_tts, "secret", config.secret);
		_sdk_config(_sdk_tts, "api_version", config.api_version);
	}

	// invoke by native poll thread
	private void handle_callback(TtsResult res) {
		assert(res.id > 0);
		TtsCallback cb;
		boolean del_cb = false;
		synchronized (_callbacks) {
			cb = _callbacks.get(res.id);
		}
		if (cb != null) {
			switch(res.type) {
				case 0:
					cb.onVoice(res.id, res.voice);
					break;
				case 1:
					cb.onStart(res.id);
					break;
				case 2:
					cb.onComplete(res.id);
					del_cb = true;
					break;
				case 3:
					cb.onCancel(res.id);
					del_cb = true;
					break;
				case 4:
					cb.onError(res.id, res.err);
					del_cb = true;
					break;
			}
		}
		if (del_cb) {
			synchronized (_callbacks) {
				_callbacks.remove(res.id);
			}
		}
	}

	private static native void _sdk_init(Class tts_cls, Class res_cls);

	private native long _sdk_create();

	private native void _sdk_delete(long sdk_tts);

	private native boolean _sdk_prepare(long sdk_tts);

	private native void _sdk_release(long sdk_tts);

	private native int _sdk_speak(long sdk_tts, String content);

	private native void _sdk_cancel(long sdk_tts, int id);

	private native void _sdk_config(long sdk_tts, String key, String value);

	private SparseArray<TtsCallback> _callbacks;

	private long _sdk_tts;

	static {
		System.loadLibrary("rokid_tts_jni");
		_sdk_init(Tts.class, TtsResult.class);
	}

	private static final String TAG = "tts.sdk";

	private static class TtsResult {
		public int id;
		// 0:  voice data
		// 1:  start voice
		// 2:  end voice
		// 3:  cancel voice
		// 4:  error
		public int type;
		public int err;
		public byte[] voice;
	}
}

class TtsConfig {
	public String server_address;

	public String ssl_roots_pem;

	public String auth_key;

	public String device_type_id;

	public String device_id;

	public String secret;

	public String api_version;

	public String toString() {
		return "server_address=" + server_address
			+ ", ssl_roots_pem=" + ssl_roots_pem
			+ ", auth_key=" + auth_key
			+ ", device_type_id=" + device_type_id
			+ ", device_id=" + device_id
			+ ", secret=" + secret
			+ ", api_version=" + api_version;
	}
}
