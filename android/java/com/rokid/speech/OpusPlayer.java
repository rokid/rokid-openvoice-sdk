package com.rokid.speech;

import android.media.AudioTrack;
import android.media.AudioFormat;
import android.media.AudioAttributes;
import android.util.Log;

public class OpusPlayer {
	public OpusPlayer() {
		_opus = new Opus(SAMPLE_RATE, PLAY_CHANNEL, 16000,
				Opus.OpusApplication.OPUS_AUDIO);
		_audioTrack = createAudioTrack();
		_audioTrack.play();
	}

	public void play(byte[] data) {
		Log.d(TAG, "play data " + data.length);
		data = _opus.decode(data);
		Log.d(TAG, "play decoded data " + data.length);

		int r;
		int offset = 0;
		int size = data.length;
		int retry = 4;
		while (offset < size) {
			r = _audioTrack.write(data, offset, size - offset);
			Log.d(TAG, "write to audio track " + r);
			if (r < 0) {
				Log.w(TAG, "audio play write data error: " + r);
				_audioTrack.release();
				_audioTrack = createAudioTrack();
				_audioTrack.play();
				break;
			}
			if (r == 0) {
				Log.d(TAG, "write to audio track returns 0, write retry count " + retry);
				if (retry == 0) {
					_audioTrack.release();
					_audioTrack = createAudioTrack();
					_audioTrack.play();
					break;
				}
				--retry;
				try {
					Thread.sleep(1000);
				} catch (Exception e) {
				}
			}
			offset += r;
		}
	}

	public void pause() {
		try {
			_audioTrack.pause();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void resume() {
		try {
			_audioTrack.play();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	private AudioTrack createAudioTrack() {
		int bufSize = AudioTrack.getMinBufferSize(SAMPLE_RATE.getValue(),
				AudioFormat.CHANNEL_OUT_MONO, AUDIO_ENCODING) * 2;
		return new AudioTrack.Builder()
			.setAudioAttributes(new AudioAttributes.Builder()
					.setUsage(AudioAttributes.USAGE_ALARM)
					.setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
					.build())
			.setAudioFormat(new AudioFormat.Builder()
					.setEncoding(AUDIO_ENCODING)
					.setSampleRate(SAMPLE_RATE.getValue())
					.setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
					.build())
			.setBufferSizeInBytes(bufSize)
			.build();
	}

	private static final Opus.OpusSampleRate SAMPLE_RATE = Opus.OpusSampleRate.SR_24K;
	private static final int PLAY_CHANNEL = 1;
	private static final int AUDIO_ENCODING = AudioFormat.ENCODING_PCM_16BIT;
	private static final String TAG = "speech.OpusPlayer";

	private AudioTrack _audioTrack;
	private Opus _opus;
}
