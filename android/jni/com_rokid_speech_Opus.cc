#include "JNIHelp.h"
#include "jni.h"
#include "log.h"
#include "rkcodec.h"
#include <cstdlib>

#define TAG "rokid.opus.jni"
#define SAMPLE_RATE 24000
#define OPU_BITRATE 16000
#define DURATION 20

namespace rokid {
namespace speech {

static jlong native_opus_decoder_create(JNIEnv *env, jobject thiz) {
	int error;
	RKOpusDecoder* decoder = new RKOpusDecoder();
	if (!decoder->init(SAMPLE_RATE, OPU_BITRATE, DURATION)) {
		Log::w(TAG, "decoder create error");
		return 0;
	}
	return (jlong)decoder;
}

static jbyteArray native_opus_decode(JNIEnv *env, jobject clazz, jlong dec, jbyteArray in) {
	RKOpusDecoder* decoder = reinterpret_cast<RKOpusDecoder*>(dec);
	if (decoder == NULL)
		return NULL;
	jbyte* opu = env->GetByteArrayElements(in, NULL);
	uint32_t length = env->GetArrayLength(in);
	uint32_t off = 0;
	uint32_t opu_frame_size = decoder->opu_frame_size();
	uint32_t frame_count = length / opu_frame_size;
	uint32_t pcm_frame_size = decoder->pcm_frame_size() * sizeof(uint16_t);
	uint32_t out_size = frame_count * pcm_frame_size;
	const uint16_t* pcm_buf;
	jbyteArray out = env->NewByteArray(out_size);
	if (out == NULL)
		return NULL;
	jbyte* pcm = env->GetByteArrayElements(out, NULL);
	uint32_t i;
	for (i = 0; i < frame_count; ++i) {
		pcm_buf = decoder->decode_frame(opu + i * opu_frame_size);
		memcpy(pcm + i * pcm_frame_size, pcm_buf, pcm_frame_size);
	}
	env->ReleaseByteArrayElements(in, opu, JNI_ABORT);
	env->ReleaseByteArrayElements(out, pcm, 0);
	return out;
}

static void native_opus_decoder_destroy(JNIEnv *env, jobject clazz, jlong dec) {
	RKOpusDecoder* decoder = reinterpret_cast<RKOpusDecoder*>(dec);
	if (decoder == NULL)
		return;
	decoder->close();
	delete decoder;
}

static JNINativeMethod method_table[] = {
    { "native_opus_decoder_create", "()J", (void*)native_opus_decoder_create },
    { "native_opus_decode", "(J[B)[B", (void*)native_opus_decode },
    { "native_opus_decoder_destroy", "(J)V", (void*)native_opus_decoder_destroy }
};

int register_com_rokid_speech_Opus(JNIEnv *env) {
	const char *kclass = "com/rokid/speech/Opus";
	jclass clazz = env->FindClass (kclass);
	if (clazz == NULL) {
		Log::e(TAG, "find class %s failed.", kclass);
		return -1;
	}
	return jniRegisterNativeMethods(env, kclass,
															 method_table, NELEM(method_table));
}

} // namespace speech
} // namespace rokid

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env;

	if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
		rokid::speech::Log::e(TAG, "JNI_OnLoad failed");
		return -1;
	}
	rokid::speech::register_com_rokid_speech_Opus(env);
	return JNI_VERSION_1_4;
}
