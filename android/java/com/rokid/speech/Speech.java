package com.rokid.speech;

import java.io.InputStream;
import android.util.Log;
import android.util.SparseArray;
import org.json.JSONObject;

public class Speech extends GenericConfig {
	public Speech() {
		_callbacks = new SparseArray<SpeechCallback>();
		_sdk_speech = _sdk_create();
	}

	public void finalize() {
		_sdk_release(_sdk_speech);
		_sdk_delete(_sdk_speech);
	}

	public void prepare(String configFile) {
		PrepareOptions opt;
		if (configFile != null)
			opt = parseConfigFile(configFile);
		else
			opt = new PrepareOptions();
		prepare(opt);
	}

	public void prepare(InputStream is) {
		PrepareOptions opt;
		if (is != null)
			opt = parseConfig(is);
		else
			opt = new PrepareOptions();
		prepare(opt);
	}

	public void prepare(PrepareOptions opt) {
		_sdk_prepare(_sdk_speech, opt);
	}

	public void release() {
		_sdk_release(_sdk_speech);
	}

	public int putText(String content, SpeechCallback cb) {
		return putText(content, cb, null);
	}

	public int putText(String content, SpeechCallback cb, VoiceOptions opt) {
		int id;
		synchronized (_callbacks) {
			id = _sdk_put_text(_sdk_speech, content, opt);
			Log.d(TAG, "put text " + content + ", id = " + id);
			if (id > 0)
				_callbacks.put(id, cb);
		}
		return id;
	}

	public int startVoice(SpeechCallback cb) {
		return startVoice(cb, null);
	}

	public int startVoice(SpeechCallback cb, VoiceOptions opt) {
		int id;
		synchronized (_callbacks) {
			id = _sdk_start_voice(_sdk_speech, opt);
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

	public void config(SpeechOptions opt) {
		_sdk_config(_sdk_speech, opt);
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
			try {
				switch(res.type) {
					case SpeechResult.INTERMEDIATE:
						cb.onIntermediateResult(res.id, res.asr, res.extra);
						break;
					case SpeechResult.START:
						cb.onStart(res.id);
						break;
					case SpeechResult.ASR_FINISH:
						cb.onAsrComplete(res.id, res.asr);
						break;
					case SpeechResult.END:
						cb.onComplete(res.id, res.nlp, res.action);
						del_cb = true;
						break;
					case SpeechResult.CANCELLED:
						cb.onCancel(res.id);
						del_cb = true;
						break;
					case SpeechResult.ERROR:
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

	private static native void _sdk_init(Class speech_cls,
			Class res_cls, Class options_cls);

	private native long _sdk_create();

	private native void _sdk_delete(long sdk_speech);

	private native boolean _sdk_prepare(long sdk_speech, PrepareOptions opt);

	private native void _sdk_release(long sdk_speech);

	private native int _sdk_put_text(long sdk_speech, String content, VoiceOptions opt);

	private native int _sdk_start_voice(long sdk_speech, VoiceOptions opt);

	private native void _sdk_put_voice(long sdk_speech, int id,
			byte[] voice, int offset, int length);

	private native void _sdk_end_voice(long sdk_speech, int id);

	private native void _sdk_cancel(long sdk_speech, int id);

	private native void _sdk_config(long sdk_speech, SpeechOptions opt);

	private SparseArray<SpeechCallback> _callbacks;

	private long _sdk_speech;

	static {
		System.loadLibrary("rokid_speech_jni");
		_sdk_init(Speech.class, SpeechResult.class, VoiceOptions.class);
	}

	private static final String TAG = "speech.sdk";

	private static class SpeechResult {
		public int id;
		public int type;
		public int err;
		public String asr;
		public String nlp;
		public String action;
		public String extra;

		private static final int INTERMEDIATE = 0;
		private static final int START = 1;
		private static final int ASR_FINISH = 2;
		private static final int END = 3;
		private static final int CANCELLED = 4;
		private static final int ERROR = 5;
	}

	public static class VoiceOptions {
		// 当前应用栈，只记录当前应用与上一应用
		public String stack;
		// 当前语音包含的激活词
		public String voice_trigger;
		// 当前语音包含的激活词开始点
		public int trigger_start;
		// 当前语音包含的激活词数据长度
		public int trigger_length;
		// 给skill传的额外参数，json格式
		//   媒体播放状态
		//   当前音乐的播放进度
		//   其它
		public String skill_options;
		// 当前语音的额外参数
		//   噪声阈值
		public String voice_extra;
	}
}
