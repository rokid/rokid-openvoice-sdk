#define LOG_TAG "OPUSNATIVE"

#include "JNIHelp.h"
#include "jni.h"
#include "utils/Log.h"
#include "utils/misc.h"
#include "cutils/log.h"

#include <cstdlib>

#include <opus.h>
#include <opus_types.h>
#include <opus_multistream.h>

namespace rokid {
namespace speech {

typedef struct {
	OpusEncoder *enc;
	OpusDecoder *dec;
	int sample_rate;
	int channels;
	int application;
	int duration;
	int opus_bitrate;
	int pcm_bitrate;
	int pcm_frame_size;
	int opus_frame_size;
} opus;

static jlong native_opus_encoder_create(JNIEnv *env, jobject clazz, int sample_rate,
                                            int channels, int bitrate, int application)
{
	int error;
	opus *encoder = (opus *)calloc(1, sizeof(opus));

	encoder->enc = opus_encoder_create(sample_rate, channels, application, &error);
	if (error != OPUS_OK) {
		ALOGW("encoder create error %s\n", opus_strerror(error));
		free(encoder);

		return 0;
	} else {
		opus_encoder_ctl(encoder->enc, OPUS_SET_VBR(0)); /* only support this */
		opus_encoder_ctl(encoder->enc, OPUS_SET_BITRATE(bitrate)); /* only support this */

		encoder->sample_rate = sample_rate;
		encoder->channels = channels;
		encoder->application = application;
		encoder->duration = 20; /* 20ms */
		encoder->opus_bitrate = bitrate;
		encoder->pcm_bitrate = sample_rate*channels*sizeof(opus_int16)*8;
		encoder->pcm_frame_size = encoder->duration * sample_rate / 1000;
		encoder->opus_frame_size = sizeof(opus_int16) * encoder->pcm_frame_size * bitrate/encoder->pcm_bitrate;

        return (jlong)encoder;
    }

}

static jlong native_opus_decoder_create(JNIEnv *env, jobject thiz, int sample_rate, int channels, int bitrate)
{
	int error;
	opus *decoder = (opus *)calloc(1, sizeof(opus));

	decoder->dec = opus_decoder_create(sample_rate, channels, &error);
	if (error != OPUS_OK) {
		printf("decoder create error %s\n", opus_strerror(error));
		free(decoder);

		return 0;
	} else {
		decoder->sample_rate = sample_rate;
		decoder->channels = channels;
		decoder->duration = 20; /* 20ms */
		decoder->opus_bitrate = bitrate;
		decoder->pcm_bitrate = sample_rate * channels * sizeof(opus_int16) * 8;
		decoder->pcm_frame_size = decoder->duration * sample_rate / 1000;
		decoder->opus_frame_size = sizeof(opus_int16) * decoder->pcm_frame_size * bitrate / decoder->pcm_bitrate;

		opus_decoder_ctl(decoder->dec, OPUS_SET_BITRATE(bitrate));
		opus_decoder_ctl(decoder->dec, OPUS_SET_VBR(0)); /* only support this */

		/* opus_decoder_ctl(dec, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND)); */

		return (jlong)decoder;
	}

}


/* DirectBuffer is needed */
static jbyteArray native_opus_encode(JNIEnv *env, jobject clazz, jlong enc, jbyteArray in)
{
    opus *encoder = (opus *)enc;
    opus_int16 *pcm = (opus_int16 *)env->GetByteArrayElements(in, 0);
    const uint32_t len = env->GetArrayLength(in);
    uint32_t pcm_frame_size = encoder->pcm_frame_size;
    uint32_t opus_frame_size = encoder->opus_frame_size;
    unsigned char *opus = new unsigned char[len*opus_frame_size*sizeof(opus_int16)/pcm_frame_size];
    ALOGD("encode len(%d), pcm_frame_size(%d) opus_frame_size(%d)", len, pcm_frame_size, opus_frame_size);

    uint32_t total_len = 0;
    int out_len = 0;
    unsigned char *opus_buf = opus;
    uint32_t encoded_size = 0;
	opus_int16 *pcm_orig = pcm;
    while (encoded_size < (len/sizeof(opus_int16)/pcm_frame_size)) {
        out_len = opus_encode(encoder->enc, pcm, pcm_frame_size, opus_buf, opus_frame_size);
        if (out_len < 0) {
            ALOGW("frame_size(%d) failed: %s", pcm_frame_size, opus_strerror(out_len));
            out_len = 0;
            break;
        } else if (out_len != (int)opus_frame_size) {
            ALOGW("Something abnormal happened out_len(%d) pcm_frame_size(%d), check it!!!",
                                                out_len, pcm_frame_size);
        }

        pcm += pcm_frame_size;
        opus_buf += out_len;
        total_len += out_len;
        encoded_size++;
    }

    env->ReleaseByteArrayElements(in, (jbyte *)pcm_orig, JNI_ABORT);

    jbyteArray out = env->NewByteArray(total_len);
    env->SetByteArrayRegion(out, 0, total_len, (jbyte *)opus);

    delete[] opus;

    return out;
}

static jbyteArray native_opus_decode(JNIEnv *env, jobject clazz, jlong dec, jbyteArray in)
{
    opus *decoder = (opus *)dec;
    unsigned char* opus = (unsigned char *)env->GetByteArrayElements(in, NULL);;
    const int len = env->GetArrayLength(in);

    int opus_frame_size = decoder->opus_frame_size;
    int pcm_frame_size = decoder->pcm_frame_size;
    int compress_ratio = sizeof(opus_int16) * decoder->pcm_frame_size/decoder->opus_frame_size;
    opus_int16 *pcm = new int16_t[len*compress_ratio];

    ALOGD("decode len(%d), compress_ratio(%d), opus_frame_size(%d), pcm_frame_size(%d)",
                            len, compress_ratio, opus_frame_size, pcm_frame_size);

    int total_len = 0;
    int decoded_size = 0;
    int out_len = 0;
	unsigned char* opus_orig = opus;
    opus_int16 *pcm_buf = pcm;
    while (decoded_size++ < (len/opus_frame_size)) {
        out_len = opus_decode(decoder->dec, opus, opus_frame_size, pcm_buf, pcm_frame_size, 0);

        if (out_len < 0) {
            ALOGW("opus decode len(%d) opus_len(%d) %s", len, opus_frame_size, opus_strerror(out_len));
            break;
        } else if (out_len != pcm_frame_size) {
            ALOGW("VBS not support!! out_len(%d) pcm_frame_size(%d)", out_len, pcm_frame_size);
            break;
        }

        opus += opus_frame_size;
        pcm_buf += out_len;
        total_len += out_len;
    }

	ALOGD("opus decoded data total len = %d", total_len);
    env->ReleaseByteArrayElements(in, (jbyte *)opus_orig, 0);

    jbyteArray out = env->NewByteArray(total_len*sizeof(opus_int16));
    env->SetByteArrayRegion(out, 0, total_len*sizeof(opus_int16), (jbyte *)pcm);

    delete[] pcm;

    return out;
}

static JNINativeMethod method_table[] = {
    { "native_opus_encoder_create", "(IIII)J", (void*)native_opus_encoder_create },
    { "native_opus_decoder_create", "(III)J", (void*)native_opus_decoder_create },
    { "native_opus_encode", "(J[B)[B", (void*)native_opus_encode },
    { "native_opus_decode", "(J[B)[B", (void*)native_opus_decode },
//    { "native_opus_encoder_ctl", "", (void*)native_opus_encoder_ctl }
//    { "native_opus_decoder_ctl", "", (void*)native_opus_decoder_ctl }
//    { "native_opus_encoder_destroy", "", (void*)native_opus_encoder_destroy }
//    { "native_opus_decoder_destroy", "", (void*)native_opus_decoder_destroy }
};

int register_com_rokid_speech_Opus(JNIEnv *env)
{

    const char *kclass = "com/rokid/speech/Opus";
    jclass clazz = env->FindClass (kclass);
    if (clazz == NULL) {
        ALOGE ("find class %s failed.", kclass);
        return -1;
    } else {
        ALOGV("Opus JNI method table create");
    }

    return jniRegisterNativeMethods(env, kclass,
                                     method_table, NELEM(method_table));
}

} // namespace speech
} // namespace rokid
