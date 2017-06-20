package com.rokid.speech;

public interface SpeechCallback {
	void onStart(int id);

	void onIntermediateResult(int id, String asr, String extra);

	void onComplete(int id, String asr, String nlp, String action);

	void onCancel(int id);

	void onError(int id, int err);
}
