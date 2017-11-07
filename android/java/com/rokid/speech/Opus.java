package com.rokid.speech;

import android.text.TextUtils;
import android.util.Log;

public class Opus {
	final static String TAG = "Opus";
	private long encoder, decoder;

	private native long native_opus_encoder_create(int sampleRate, int channels, int bitrate, int application);
	private native byte[] native_opus_encode(long encoder, byte[] in);
	private native long native_opus_decoder_create(int sampleRate, int channels, int bitrate);
	private native byte[] native_opus_decode(long decoder, byte[] in);
	public enum OpusSampleRate {
		SR_8K(8000), SR_16K(16000), SR_24K(24000), SR_48K(48000);

		private int sampleRate;
		public int getValue() {
			return this.sampleRate;
		}
		OpusSampleRate(int i) {
			sampleRate = i;
		}
	}

	public enum OpusApplication {
		OPUS_AUDIO("audio"), OPUS_VOIP("voip");

		private int application;

		int getValue() {
			return this.application;
		}

		final static int OPUS_APPLICATION_AUDIO = 2048;
		final static int OPUS_APPLICATION_VOIP = 2049;
		OpusApplication(String s) {
			if (TextUtils.equals(s, "audio"))
				application = OPUS_APPLICATION_AUDIO;
			else
				application = OPUS_APPLICATION_VOIP;  /* voip */
		}

	}


	public Opus(OpusSampleRate sr, int ch, int bitrate, OpusApplication app) {
		int sampleRate = sr.getValue();
		int channels = ch;

		if ((decoder=native_opus_decoder_create(sampleRate, channels, bitrate)) == 0) {
			Log.e(TAG, "decoder create failed"); /* agly */
		}

		int application = app.getValue();
		if ((encoder=native_opus_encoder_create(sampleRate, channels, bitrate, application)) == 0) {
			Log.e(TAG, "encoder create failed");
		}
	}

	public byte[] encode(byte[] in) {
		byte[] out;

		out = native_opus_encode(encoder, in);
		if (out.length == 0) {
			Log.e(TAG, "encode error");
			return null;
		}

		return out;
	}

	public byte[] decode(byte[] in) {
		byte[] out;

		out = native_opus_decode(decoder, in);
		if (out.length == 0) {
			Log.e(TAG, "decode failed");

			return null;
		}

		return out;
	}

	static {
		System.loadLibrary("rokid_opus_jni");
	}
}
