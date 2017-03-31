package com.rokid.speech;

public interface SpeechCallback {
	void onStart(int id);

	void onAsr(int id, String asr);

	void onNlp(int id, String nlp);

	void onAction(int id, String action);

	void onComplete(int id);

	void onStop(int id);

	void onError(int id, int err);
}
