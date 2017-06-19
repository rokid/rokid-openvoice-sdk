package com.rokid.speech.example;

import java.io.FileInputStream;
import java.io.IOException;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import com.rokid.speech.Asr;
import com.rokid.speech.AsrCallback;

public class AsrDemo extends Service implements Runnable {
	@Override
	public void onCreate() {
		_asr = new Asr("/system/etc/speech_sdk.json");
		_asr.prepare();
		_asr.config("codec", "pcm");
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
		new Thread(this).start();
		return START_NOT_STICKY;
	}

	@Override
	public void run() {
		AsrCallback cb = new AsrCallbackDemo();
		int i;
		for (i = 0; i < TEST_ASR_COUNT; ++i) {
			doAsrRequest(cb);
		}
		Log.d(TAG, "asr test thread quit");
	}

	private void doAsrRequest(AsrCallback cb) {
		FileInputStream is = null;
		byte[] buf = new byte[4096];
		int c;
		int id;

		id = _asr.startVoice(cb);
		try {
			is = new FileInputStream(TEST_ASR_VOICE_FILE);
			while (true) {
				c = is.read(buf);
				if (c > 0) {
					_asr.putVoice(id, buf, 0, c);
				} else
					break;
			}
		} catch (IOException e) {
			e.printStackTrace();
			return;
		} finally {
			try {
				if (is != null)
					is.close();
			} catch (Exception e) {
			}
		}
		_asr.endVoice(id);
	}

	public static final String TAG = "AsrDemo";
	private static final int TEST_ASR_COUNT = 100;
	private static final String TEST_ASR_VOICE_FILE = "/data/speech_demo/hello.pcm";

	private Asr _asr;
}

class AsrCallbackDemo implements AsrCallback {
	public void onStart(int id) {
		Log.d(AsrDemo.TAG, "onStart " + id);
	}

	public void onAsr(int id, String asr) {
		Log.d(AsrDemo.TAG, "onAsr " + id + ", " + asr);
	}

	public void onComplete(int id) {
		Log.d(AsrDemo.TAG, "onComplete " + id);
	}

	public void onCancel(int id) {
		Log.d(AsrDemo.TAG, "onCancel " + id);
	}

	public void onError(int id, int err) {
		Log.d(AsrDemo.TAG, "onError id " + id + ", err " + err);
	}
}
