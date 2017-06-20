package com.rokid.speech;

public interface AsrCallback {
	void onStart(int id);

	void onIntermediateResult(int id, String asr);

	void onComplete(int id, String asr);

	void onCancel(int id);

	void onError(int id, int err);
}
