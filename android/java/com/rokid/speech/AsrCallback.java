package com.rokid.speech;

public interface AsrCallback {
	void onStart(int id);

	void onAsr(int id, String asr);

	void onComplete(int id);

	void onCancel(int id);

	void onError(int id, int err);
}
