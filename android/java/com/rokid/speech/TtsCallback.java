package com.rokid.speech;

public interface TtsCallback {
	public void onStart(int id);

	public void onVoice(int id, byte[] data, String text);

	public void onCancel(int id);

	public void onComplete(int id);

	public void onError(int id, int err);
}
