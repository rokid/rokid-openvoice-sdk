package com.rokid.speech.example;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import com.rokid.speech.Speech;
import com.rokid.speech.SpeechCallback;

public class SpeechDemo extends Service {
	@Override
	public void onCreate() {
		_speech = new Speech("/system/etc/speech_sdk.json");
		_speech.config("codec", "opu");
	}

	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}

	@Override
	public void onDestroy() {
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		SpeechCallback cb = new SpeechCallbackDemo();
		_speech.putText("若琪你好", cb);
		return START_NOT_STICKY;
	}

	public static final String TAG = "SpeechDemo";

	private Speech _speech;
}

class SpeechCallbackDemo implements SpeechCallback {
	public void onStart(int id) {
		Log.d(SpeechDemo.TAG, "onStart " + id);
	}

	public void onAsr(int id, String asr) {
		Log.d(SpeechDemo.TAG, "onAsr " + id + ", " + asr);
	}

	public void onNlp(int id, String nlp) {
		Log.d(SpeechDemo.TAG, "onNlp " + id + ", " + nlp);
	}

	public void onAction(int id, String action) {
		Log.d(SpeechDemo.TAG, "onAction " + id + ", " + action);
	}

	public void onComplete(int id) {
		Log.d(SpeechDemo.TAG, "onComplete " + id);
	}

	public void onCancel(int id) {
		Log.d(SpeechDemo.TAG, "onCancel " + id);
	}

	public void onError(int id, int err) {
		Log.d(SpeechDemo.TAG, "onError id " + id + ", err " + err);
	}
}
