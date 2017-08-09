package com.rokid.speech;

public class SpeechOptions {
	public SpeechOptions() {
		_native_options = native_new_options();
	}

	public void finalize() {
		native_release(_native_options);
	}

	public void release() {
		native_release(_native_options);
		_native_options = 0;
	}

	// lang: "zh" "en"
	public void set_lang(String lang) {
		if (_native_options != 0)
			native_set_lang(_native_options, lang);
	}

	// codec: "pcm" "opu"
	public void set_codec(String codec) {
		if (_native_options != 0)
			native_set_codec(_native_options, codec);
	}

	// mode: "local" "cloud"
	public void set_vad_mode(String mode) {
		if (_native_options != 0)
			native_set_vad_mode(_native_options, mode);
	}

	public void set_no_nlp(boolean v) {
		if (_native_options != 0)
			native_set_no_nlp(_native_options, v);
	}

	public void set_no_intermediate_asr(boolean v) {
		if (_native_options != 0)
			native_set_no_intermediate_asr(_native_options, v);
	}

	private native long native_new_options();

	private native void native_release(long opt);

	private native void native_set_lang(long opt, String v);

	private native void native_set_codec(long opt, String v);

	private native void native_set_vad_mode(long opt, String v);

	private native void native_set_no_nlp(long opt, boolean v);

	private native void native_set_no_intermediate_asr(long opt, boolean v);

	private long _native_options;

	static {
		System.loadLibrary("rokid_speech_jni");
	}
}
