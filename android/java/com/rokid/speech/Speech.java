package com.rokid.speech;

import java.io.FileInputStream;
import java.io.IOException;
import android.util.SparseArray;
import android.util.Log;
import com.alibaba.fastjson.JSON;

public class Speech implements Runnable {
	public Speech(String configFile) {
		_callbacks = new SparseArray<SpeechCallback>();
		_sdk_speech = _sdk_create();
		callbackThread = new Thread(this);

		config_speech(configFile);
	}

	public void finalize() {
		_sdk_delete(_sdk_speech);
		try {
			callbackThread.join();
		} catch (Exception e) {
		}
	}

	public boolean prepare() {
		return _sdk_prepare(_sdk_speech);
	}

	public void release() {
		_sdk_release(_sdk_speech);
	}

	public int putText(String text, SpeechCallback cb) {
		int id;
		if (!ready())
			return -1;
		synchronized (_callbacks) {
			id = _sdk_put_text(_sdk_speech, text);
			if (id > 0)
				_callbacks.put(id, cb);
		}
		return id;
	}

	public int startVoice(SpeechCallback cb) {
		int id;
		if (!ready())
			return -1;
		synchronized (_callbacks) {
			id = _sdk_start_voice(_sdk_speech);
			if (id > 0)
				_callbacks.put(id, cb);
		}
		return id;
	}

	public void putVoice(int id, byte[] data) {
		if (!ready())
			return;
		_sdk_put_voice(_sdk_speech, id, data, 0, data.length);
	}

	public void putVoice(int id, byte[] data, int offset, int length) {
		if (!ready())
			return;
		_sdk_put_voice(_sdk_speech, id, data, offset, length);
	}

	public void endVoice(int id) {
		if (!ready())
			return;
		_sdk_end_voice(_sdk_speech, id);
	}

	public void cancel(int id) {
		if (!ready())
			return;
		_sdk_cancel(_sdk_speech, id);
	}

	public void config(String key, String value) {
		if (!ready())
			return;
		_sdk_config(_sdk_speech, key, value);
	}

	public boolean ready() {
		return _sdk_speech != 0;
	}

	@Override
	public void run() {
		SpeechResult sr;
		SpeechCallback cb;

		while (true) {
			sr = _sdk_poll(_sdk_speech);
			// sdk speech released
			if (sr == null)
				break;
			synchronized (_callbacks) {
				cb = _callbacks.get(sr.id);
				if (cb == null) {
					Log.w(TAG, "speech " + sr.id + " has no callback, iginore");
					continue;
				}
			}
			switch (sr.type) {
				// asr, nlp, action result
				case 0:
					if (sr.asr != null && sr.asr.length() > 0)
						cb.onAsr(sr.id, sr.asr);
					if (sr.nlp != null && sr.nlp.length() > 0)
						cb.onNlp(sr.id, sr.nlp);
					if (sr.action != null && sr.action.length() > 0)
						cb.onAction(sr.id, sr.action);
					break;
				// speech start
				case 1:
					cb.onStart(sr.id);
					break;
				// speech complete
				case 2:
					cb.onComplete(sr.id);
					break;
				// speech cancelled
				case 3:
					cb.onStop(sr.id);
					break;
				// speech error
				case 4:
					cb.onError(sr.id, sr.err);
					break;
				default:
					Log.w(TAG, "unknow speech result type " + sr.type);
			}
		}
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
		_sdk_config(_sdk_speech, "server_address", config.server_address);
		_sdk_config(_sdk_speech, "ssl_roots_pem", config.ssl_roots_pem);
		_sdk_config(_sdk_speech, "auth_key", config.auth_key);
		_sdk_config(_sdk_speech, "device_type", config.device_type);
		_sdk_config(_sdk_speech, "device_id", config.device_id);
		_sdk_config(_sdk_speech, "secret", config.secret);
		_sdk_config(_sdk_speech, "api_version", config.api_version);
	}

	private native long _sdk_create();

	private native void _sdk_delete(long speech);

	private native boolean _sdk_prepare(long speech);

	private native void _sdk_release(long speech);

	private native int _sdk_put_text(long speech, String text);

	private native int _sdk_start_voice(long speech);

	private native void _sdk_put_voice(long speech, int id, byte[] data, int offset, int length);

	private native void _sdk_end_voice(long speech, int id);

	private native void _sdk_cancel(long speech, int id);

	private native void _sdk_config(long speech, String key, String value);

	private native SpeechResult _sdk_poll(long speech);

	private static final String TAG = "speech.sdk";
	private long _sdk_speech;
	private SparseArray<SpeechCallback> _callbacks;
	private Thread callbackThread;

	static {
		System.loadLibrary("rokid_speech_jni");
	}
}

class SpeechConfig {
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
