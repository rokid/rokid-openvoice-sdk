package com.rokid.speech;

import android.util.Log;
import android.util.SparseArray;

public class Asr extends GenericConfig<AsrConfig> {
	public Asr(String configFile) {
		_callbacks = new SparseArray<AsrCallback>();
		_sdk_asr = _sdk_create();
		if (configFile != null)
			config(configFile, AsrConfig.class);
	}

	public void finalize() {
		_sdk_release(_sdk_asr);
		_sdk_delete(_sdk_asr);
	}

	public void prepare() {
		_sdk_prepare(_sdk_asr);
	}

	public void release() {
		_sdk_release(_sdk_asr);
	}

	public int startVoice(AsrCallback cb) {
		int id;
		synchronized (_callbacks) {
			id = _sdk_start_voice(_sdk_asr);
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
		_sdk_put_voice(_sdk_asr, id, data, offset, length);
	}

	public void endVoice(int id) {
		_sdk_end_voice(_sdk_asr, id);
	}

	public void cancel(int id) {
		_sdk_cancel(_sdk_asr, id);
	}

	public void config(String key, String value) {
		_sdk_config(_sdk_asr, key, value);
	}

	// invoke by native poll thread
	private void handle_callback(AsrResult res) {
		assert(res.id > 0);
		AsrCallback cb;
		boolean del_cb = false;
		synchronized (_callbacks) {
			cb = _callbacks.get(res.id);
		}
		if (cb != null) {
			try {
				switch(res.type) {
					case AsrResult.INTERMEDIATE:
						if (res.asr != null && res.asr.length() > 0)
							cb.onIntermediateResult(res.id, res.asr);
						break;
					case AsrResult.START:
						cb.onStart(res.id);
						break;
					case AsrResult.END:
						cb.onComplete(res.id, res.asr);
						del_cb = true;
						break;
					case AsrResult.CANCELLED:
						cb.onCancel(res.id);
						del_cb = true;
						break;
					case AsrResult.ERROR:
						cb.onError(res.id, res.err);
						del_cb = true;
						break;
				}
			} catch (Exception e) {
				Log.w(TAG, "invoke callback through binder occurs exception");
				e.printStackTrace();
				del_cb = true;
			}
		}
		if (del_cb) {
			synchronized (_callbacks) {
				_callbacks.remove(res.id);
			}
		}
	}

	private static native void _sdk_init(Class asr_cls, Class res_cls);

	private native long _sdk_create();

	private native void _sdk_delete(long sdk_asr);

	private native boolean _sdk_prepare(long sdk_asr);

	private native void _sdk_release(long sdk_asr);

	private native int _sdk_start_voice(long sdk_asr);

	private native void _sdk_put_voice(long sdk_asr, int id, byte[] voice, int offset, int length);

	private native void _sdk_end_voice(long sdk_asr, int id);

	private native void _sdk_cancel(long sdk_asr, int id);

	private native void _sdk_config(long sdk_asr, String key, String value);

	private SparseArray<AsrCallback> _callbacks;

	private long _sdk_asr;

	static {
		System.loadLibrary("rokid_speech_jni");
		_sdk_init(Asr.class, AsrResult.class);
	}

	private static final String TAG = "asr.sdk";

	private static class AsrResult {
		public int id;
		public int type;
		public int err;
		public String asr;

		private static final int INTERMEDIATE = 0;
		private static final int START = 1;
		private static final int END = 2;
		private static final int CANCELLED = 3;
		private static final int ERROR = 4;
	}
}

class AsrConfig extends GenericConfigParams {
}
