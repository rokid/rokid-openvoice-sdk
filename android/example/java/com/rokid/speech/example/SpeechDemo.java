package com.rokid.speech.example;

import java.io.FileInputStream;
import java.io.IOException;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import com.rokid.speech.Speech;
import com.rokid.speech.SpeechCallback;

public class SpeechDemo extends Service implements Runnable {
	@Override
	public void onCreate() {
		_speech = new Speech("/system/etc/speech_sdk.json");
		_speech.config("codec", "pcm");
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
		_speech.release();
		_speech.prepare();
		Thread th = new Thread(this);
		th.start();
		try {
			th.join();
		} catch (Exception e) {
		}
		return START_NOT_STICKY;
	}

	@Override
	public void run() {
		int i;
		SpeechCallback cb = new SpeechCallbackDemo();
		for (i = 0; i < TEST_SPEECH_COUNT; ++i) {
			doSpeechRequest(cb);
		}
	}

	private void doSpeechRequest(SpeechCallback cb) {
		FileInputStream is = null;
		byte[] buf = new byte[4096];
		int c;
		int id;

		id = _speech.startVoice(cb);
		try {
			is = new FileInputStream(TEST_SPEECH_VOICE_FILE);
			while (true) {
				c = is.read(buf);
				if (c > 0) {
					_speech.putVoice(id, buf, 0, c);
				} else
					break;
			}
		} catch (IOException e) {
			e.printStackTrace();
			return;
		} finally {
			_speech.endVoice(id);
			try {
				if (is != null)
					is.close();
			} catch (Exception e) {
			}
		}
	}


	public static final String TAG = "SpeechDemo";
	private static final int TEST_SPEECH_COUNT = 100;
	private static final String TEST_SPEECH_VOICE_FILE = "/data/speech_demo/hello.pcm";

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
