package com.rokid.speech;

import android.text.TextUtils;
import android.util.Log;

public class Opus {
	final static String TAG = "RKOpus";
	private long decoder;

	private native long native_opus_decoder_create();
	private native byte[] native_opus_decode(long decoder, byte[] in);
	private native void native_opus_decoder_destroy(long decoder);

	public void finalize() {
		if (decoder != 0)
			native_opus_decoder_destroy(decoder);
	}

	public byte[] decode(byte[] in) {
		byte[] out = null;
		if (decoder == 0)
			decoder = native_opus_decoder_create();
		if (decoder != 0)
			out = native_opus_decode(decoder, in);
		return out;
	}

	static {
		System.loadLibrary("rokid_opus_jni");
	}
}
