package com.rokid.speech;

import java.io.FileInputStream;
import java.io.IOException;
import android.util.Log;
import android.util.SparseArray;
import com.alibaba.fastjson.JSON;

public class Speech {
	public Speech(String configFile) {
		_callbacks = new SparseArray<SpeechCallback>();
		_sdk_speech = _sdk_create();
		config_speech(configFile);
		_sdk_prepare(_sdk_speech);
	}

	public void finalize() {
		_sdk_release(_sdk_speech);
		_sdk_delete(_sdk_speech);
	}

	public int putText(String content, SpeechCallback cb) {
		int id;
		synchronized (_callbacks) {
			id = _sdk_put_text(_sdk_speech, content);
			Log.d(TAG, "put text " + content + ", id = " + id);
			if (id > 0)
				_callbacks.put(id, cb);
		}
		return id;
	}

	public int startVoice(SpeechCallback cb) {
		int id;
		synchronized (_callbacks) {
			id = _sdk_start_voice(_sdk_speech);
			Log.d(TAG, "start voice, id = " + id);
			if (id > 0)
				_callbacks.put(id, cb);
		}
		return id;
	}

	public void putVoice(int id, byte[] data) {
		putVoice(id, data, 0, data.length);
	}

	public void putVoice(int id, byte[] data, int offset, int length) {
		_sdk_put_voice(_sdk_speech, id, data, offset, length);
	}

	public void endVoice(int id) {
		_sdk_end_voice(_sdk_speech, id);
	}

	public void cancel(int id) {
		_sdk_cancel(_sdk_speech, id);
	}

	public void config(String key, String value) {
		_sdk_config(_sdk_speech, key, value);
	}

	public void release() {
		_sdk_release(_sdk_speech);
	}

	private void config_speech(String configFile) {
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
		SpeechConfig config = JSON.parseObject(content, SpeechConfig.class);
		Log.d(TAG, "config file parse result = " + config);
		_sdk_config(_sdk_speech, "host", config.host);
		_sdk_config(_sdk_speech, "port", config.port);
		_sdk_config(_sdk_speech, "branch", config.branch);
		_sdk_config(_sdk_speech, "ssl_roots_pem", config.ssl_roots_pem);
		_sdk_config(_sdk_speech, "key", config.key);
		_sdk_config(_sdk_speech, "device_type_id", config.device_type_id);
		_sdk_config(_sdk_speech, "device_id", config.device_id);
		_sdk_config(_sdk_speech, "secret", config.secret);
		_sdk_config(_sdk_speech, "api_version", config.api_version);
	}

	// invoke by native poll thread
	private void handle_callback(SpeechResult res) {
		assert(res.id > 0);
		SpeechCallback cb;
		boolean del_cb = false;
		synchronized (_callbacks) {
			cb = _callbacks.get(res.id);
		}
		if (cb != null) {
			switch(res.type) {
				case 0:
					if (res.asr != null && res.asr.length() > 0)
						cb.onAsr(res.id, res.asr);
					if (res.nlp != null && res.nlp.length() > 0)
						cb.onNlp(res.id, res.nlp);
					if (res.action != null && res.action.length() > 0)
						cb.onAction(res.id, res.action);
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

	private static native void _sdk_init(Class speech_cls, Class res_cls);

	private native long _sdk_create();

	private native void _sdk_delete(long sdk_speech);

	private native boolean _sdk_prepare(long sdk_speech);

	private native void _sdk_release(long sdk_speech);

	private native int _sdk_put_text(long sdk_speech, String content);

	private native int _sdk_start_voice(long sdk_speech);

	private native void _sdk_put_voice(long sdk_speech, int id, byte[] voice, int offset, int length);

	private native void _sdk_end_voice(long sdk_speech, int id);

	private native void _sdk_cancel(long sdk_speech, int id);

	private native void _sdk_config(long sdk_speech, String key, String value);

	private SparseArray<SpeechCallback> _callbacks;

	private long _sdk_speech;

	static {
		System.loadLibrary("rokid_speech_jni");
		_sdk_init(Speech.class, SpeechResult.class);
	}

	private static final String TAG = "speech.sdk";

	private static class SpeechResult {
		public int id;
		// 0:  speech result
		// 1:  start speech
		// 2:  end speech
		// 3:  cancel speech
		// 4:  error
		public int type;
		public int err;
		public String asr;
		public String nlp;
		public String action;
	}
}

class SpeechConfig {
	public String host;

	public String port;

	public String branch;

	public String ssl_roots_pem;

	public String key;

	public String device_type_id;

	public String device_id;

	public String secret;

	public String api_version;

	public String toString() {
		return "host=" + host
			+ ":" + port
			+ "" + branch
			+ ", ssl_roots_pem=" + ssl_roots_pem
			+ ", key=" + key
			+ ", device_type_id=" + device_type_id
			+ ", device_id=" + device_id
			+ ", secret=" + secret
			+ ", api_version=" + api_version;
	}
}
