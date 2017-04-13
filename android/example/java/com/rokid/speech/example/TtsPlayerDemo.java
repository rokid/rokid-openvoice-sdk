package com.rokid.speech.example;

import android.app.Service;
import android.media.AudioTrack;
import android.media.AudioFormat;
import android.media.AudioAttributes;
import android.util.Log;
import com.rokid.speech.TtsCallback;
// import com.rokid.speech.pus;

public class TtsPlayerDemo implements TtsCallback {
	public TtsPlayerDemo(Service svc) {
		// _opus = new Opus(SAMPLE_RATE, PLAY_CHANNEL, 16000,
		//		Opus.OpusApplication.OPUS_AUDIO);
		_audioTrack = createAudioTrack();
		_audioTrack.play();
		_thisService = svc;
	}

	@Override
	public void onStart(int id) {
	}

	@Override
	public void onText(int id, String text) {
	}

	@Override
	public void onVoice(int id, byte[] data) {
		Log.d(TAG, "onVoice data " + data.length);
		// data = _opus.decode(data);
		// Log.d(TAG, "onVoice decoded data " + data.length);

		int r;
		int offset = 0;
		int size = data.length;
		while (offset < size) {
			r = _audioTrack.write(data, offset, size - offset);
			Log.d(TAG, "write to audio track " + r);
			if (r < 0) {
				Log.w(TAG, "audio play write data error: " + r);
				break;
			}
			offset += r;
		}
	}

	@Override
	public void onCancel(int id) {
		doComplete();
	}

	@Override
	public void onComplete(int id) {
		doComplete();
	}

	@Override
	public void onError(int id, int err) {
		doComplete();
	}

	private AudioTrack createAudioTrack() {
		int bufSize = AudioTrack.getMinBufferSize(SAMPLE_RATE,
				AudioFormat.CHANNEL_OUT_MONO, AUDIO_ENCODING) * 2;
		return new AudioTrack.Builder()
			.setAudioAttributes(new AudioAttributes.Builder()
					.setUsage(AudioAttributes.USAGE_ALARM)
					.setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
					.build())
			.setAudioFormat(new AudioFormat.Builder()
					.setEncoding(AUDIO_ENCODING)
					.setSampleRate(SAMPLE_RATE)
					.setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
					.build())
			.setBufferSizeInBytes(bufSize)
			.build();
	}

	private void doComplete() {
		_thisService.stopSelf();
	}

	// private static final Opus.OpusSampleRate SAMPLE_RATE = Opus.OpusSampleRate.SR_24K;
	private static final int SAMPLE_RATE = 24000;
	private static final int PLAY_CHANNEL = 1;
	private static final int AUDIO_ENCODING = AudioFormat.ENCODING_PCM_16BIT;
	private static final String TAG = "tts.PlayerDemo";

	private AudioTrack _audioTrack;
	// private Opus _opus;
	private Service _thisService;
}
