package com.rokid.speech;

public interface TtsCallback {
	public void onStart(int id);

	public void onText(int id, String text);

	public void onVoice(int id, byte[] data);

	public void onStop(int id);

	public void onComplete(int id);

	public void onError(int id, int err);
}
