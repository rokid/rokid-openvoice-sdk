package com.rokid.speech.example;

import java.io.FileInputStream;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import com.rokid.speech.Tts;

public class TtsDemo extends Service {
	@Override
	public void onCreate() {
		_tts_callback = new TtsPlayerDemo();
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
		Log.d(TAG, "TtsDemo onStart");
		if (_tts != null)
			_tts.release();

		_tts = new Tts();
		_tts.prepare("/system/etc/tts_sdk.json");

		String content = getTtsText();

		_tts.speak(content, _tts_callback);
		return START_NOT_STICKY;
	}

	private String getTtsText() {
		FileInputStream in;
		int c;
		byte[] buf;

		try {
			in = new FileInputStream(TTS_DEMO_TEXT_FILE);
			c = in.available();
			if (c > MAX_TTS_DEMO_TEXT)
				c = MAX_TTS_DEMO_TEXT;
			buf = new byte[c];
			in.read(buf);
		} catch (Exception e) {
			e.printStackTrace();
			return DEFAULT_TTS_DEMO_TEXT;
		}
		return new String(buf);
	}

	private Tts _tts;
	private TtsPlayerDemo _tts_callback;
	private static final String TAG = "tts.Demo";
	private static final String TTS_DEMO_TEXT_FILE = "/data/speech_demo/tts_demo_txt";
	private static final String DEFAULT_TTS_DEMO_TEXT = "[0.8 3:G3 0.5;C4 0.5;C4 0.25;C4 1.25;C4 0.5;C4 0.5;B3 0.5;A3 0.5;G3 0.5;B3 0.5;C4 0.5;D4 0.5 C4 0.25;C4 0.5;G3 0.5;C4 0.5;C4 0.25;C4 1.25;C4 0.5;C4 0.5;B3 0.5;A3 0.5;G3 0.5;D4 0.5;E4 0.5;F4 0.25 E4 0.5;E4 0.75]雨下整夜，我的爱溢出就像雨水院子落叶，跟我的思念厚厚一叠";
	private static final int MAX_TTS_DEMO_TEXT = 8192;
}
