package com.rokid.speech;

public class TtsOptions {
	public TtsOptions() {
		_tts_options = native_new_options();
	}

	public void finialize() {
		native_release(_tts_options);
	}

	public void release() {
		native_release(_tts_options);
		_tts_options = 0;
	}

	// codec: "pcm" "opu2", "mp3"
	public void set_codec(String codec) {
		if (_tts_options != 0)
			native_set_codec(_tts_options, codec);
	}

	// declaimer: "zh"
	public void set_declaimer(String declaimer) {
		if (_tts_options != 0)
			native_set_declaimer(_tts_options, declaimer);
	}

	public void set_samplerate(int samplerate) {
		if (_tts_options != 0)
			native_set_samplerate(_tts_options, samplerate);
	}

	private native long native_new_options();

	private native void native_release(long opt);

	private native void native_set_codec(long opt, String c);

	private native void native_set_declaimer(long opt, String d);

	private native void native_set_samplerate(long opt, int samplerate);

	private long _tts_options;

	static {
		System.loadLibrary("rokid_speech_jni");
	}
}
