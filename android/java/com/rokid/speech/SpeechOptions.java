package com.rokid.speech;

public class SpeechOptions {
	public SpeechOptions() {
		_native_options = native_new_options();
	}

	public void finalize() {
		release();
	}

	public void set(String key, String value) {
		native_set(_native_options, key, value);
	}

	public void clear() {
		native_clear(_native_options);
	}

	public void release() {
		if (_native_options != 0) {
			native_release(_native_options);
			_native_options = 0;
		}
	}

	private native long native_new_options();

	private native void native_release(long opt);

	private native void native_set(long opt, String key, String value);

	private native void native_clear(long opt);

	private long _native_options;

	static {
		System.loadLibrary("rokid_speech_jni");
	}
}
