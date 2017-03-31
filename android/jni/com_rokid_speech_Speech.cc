#include <jni.h>
#include "log.h"
#include "speech.h"
#include "JNIHelp.h"

namespace rokid {
namespace speech {

static const char* tag_ = "speech.jni";
static JavaVM* vm_ = NULL;
static jclass speech_result_class_ = NULL;
static jmethodID speech_result_constructor_ = 0;
static jfieldID speech_result_fields_[] = {
	0, 0, 0, 0, 0, 0
};
enum SpeechResultFieldName {
	SpeechResultField_id,
	SpeechResultField_type,
	SpeechResultField_err,
	SpeechResultField_asr,
	SpeechResultField_nlp,
	SpeechResultField_action,
};

static jlong com_rokid_speech_Speech__sdk_create(JNIEnv *env, jobject thiz) {
	return (jlong)new_speech();
}

static void com_rokid_speech_Speech__sdk_delete(JNIEnv *env, jobject thiz, jlong speechl) {
	Speech* speech = reinterpret_cast<Speech*>(speechl);
	if (speech == NULL)
		return;
	delete_speech(speech);
}

static jboolean com_rokid_speech_Speech__sdk_prepare(JNIEnv *env, jobject thiz, jlong speechl) {
	Speech* speech = reinterpret_cast<Speech*>(speechl);
	if (speech == NULL)
		return false;
	if (!speech->prepare()) {
		Log::d(tag_, "prepare failed");
		return false;
	}
	return true;
}

static void com_rokid_speech_Speech__sdk_release(JNIEnv *env, jobject thiz, jlong speechl) {
	Speech* speech = reinterpret_cast<Speech*>(speechl);
	if (speech == NULL)
		return;
	speech->release();
	delete speech;
}

static jint com_rokid_speech_Speech__sdk_put_text(JNIEnv *env, jobject thiz, jlong speechl, jstring text) {
	Speech* speech = reinterpret_cast<Speech*>(speechl);
	if (speech == NULL || text == NULL)
		return 0;
	const char* content = env->GetStringUTFChars(text, NULL);
	jint id = speech->put_text(content);
	env->ReleaseStringUTFChars(text, content);
	return id;
}

static jint com_rokid_speech_Speech__sdk_start_voice(JNIEnv *env, jobject thiz, jlong speechl) {
	Speech* speech = reinterpret_cast<Speech*>(speechl);
	if (speech == NULL)
		return 0;
	return speech->start_voice();
}

static void com_rokid_speech_Speech__sdk_put_voice(JNIEnv *env, jobject thiz, jlong speechl,
		jint id, jbyteArray voice, jint offset, jint length) {
	Speech* speech = reinterpret_cast<Speech*>(speechl);
	if (speech == NULL || voice == NULL || offset < 0 || length <= 0)
		return;
	jbyte* buf = new jbyte[length];
	env->GetByteArrayRegion(voice, offset, length, buf);
	if (env->ExceptionOccurred()) {
		env->ExceptionClear();
		delete buf;
		return;
	}
	speech->put_voice(id, reinterpret_cast<uint8_t*>(buf), length);
	delete buf;
}

static void com_rokid_speech_Speech__sdk_end_voice(JNIEnv *env, jobject thiz, jlong speechl, jint id) {
	Speech* speech = reinterpret_cast<Speech*>(speechl);
	if (speech == NULL)
		return;
	speech->end_voice(id);
}

static void com_rokid_speech_Speech__sdk_cancel(JNIEnv *env, jobject thiz, jlong speechl, jint id) {
	Speech* speech= reinterpret_cast<Speech*>(speechl);
	if (speech == NULL)
		return;
	speech->cancel(id);
}

static void com_rokid_speech_Speech__sdk_config(JNIEnv *env, jobject thiz,
		jlong speechl, jstring key, jstring value) {
	Speech* speech = reinterpret_cast<Speech*>(speechl);
	if (speech == NULL)
		return;
	const char* keystr = env->GetStringUTFChars(key, NULL);
	const char* valstr = env->GetStringUTFChars(value, NULL);
	speech->config(keystr, valstr);
	env->ReleaseStringUTFChars(key, keystr);
	env->ReleaseStringUTFChars(value, valstr);
}

static jobject com_rokid_speech_Speech__sdk_poll(JNIEnv *env, jobject thiz, jlong speechl) {
	Speech* speech = reinterpret_cast<Speech*>(speechl);
	if (speech == NULL)
		return NULL;
	SpeechResult sr;
	int32_t r = speech->poll(sr);
	if (r < 0)
		return NULL;
	jobject res = env->NewObject(speech_result_class_, speech_result_constructor_);
	env->SetIntField(res, speech_result_fields_[SpeechResultField_id], sr.id);
	env->SetIntField(res, speech_result_fields_[SpeechResultField_type], r);
	env->SetIntField(res, speech_result_fields_[SpeechResultField_err], sr.err);
	jstring str;
	if (sr.asr.length()) {
		str = env->NewStringUTF(sr.asr.c_str());
		env->SetObjectField(res, speech_result_fields_[SpeechResultField_asr], str);
	}
	if (sr.nlp.length()) {
		str = env->NewStringUTF(sr.nlp.c_str());
		env->SetObjectField(res, speech_result_fields_[SpeechResultField_nlp], str);
	}
	if (sr.action.length()) {
		str = env->NewStringUTF(sr.action.c_str());
		env->SetObjectField(res, speech_result_fields_[SpeechResultField_action], str);
	}
	return res;
}

static void prepare_speech_result_info(JNIEnv* env, jclass cls) {
	speech_result_constructor_ = env->GetMethodID(cls, "<init>", "()V");
	speech_result_fields_[SpeechResultField_id] = env->GetFieldID(cls, "id", "I");
	speech_result_fields_[SpeechResultField_type] = env->GetFieldID(cls, "type", "I");
	speech_result_fields_[SpeechResultField_err] = env->GetFieldID(cls, "err", "I");
	speech_result_fields_[SpeechResultField_asr] = env->GetFieldID(cls, "asr", "Ljava/lang/String;");
	speech_result_fields_[SpeechResultField_nlp] = env->GetFieldID(cls, "nlp", "Ljava/lang/String;");
	speech_result_fields_[SpeechResultField_action] = env->GetFieldID(cls, "action", "Ljava/lang/String;");
}

static JNINativeMethod _nmethods[] = {
	{ "_sdk_create", "()J", (void*)com_rokid_speech_Speech__sdk_create },
	{ "_sdk_delete", "(J)V", (void*)com_rokid_speech_Speech__sdk_delete },
	{ "_sdk_prepare", "(J)Z", (void*)com_rokid_speech_Speech__sdk_prepare },
	{ "_sdk_release", "(J)V", (void*)com_rokid_speech_Speech__sdk_release },
	{ "_sdk_put_text", "(JLjava/lang/String;)I", (void*)com_rokid_speech_Speech__sdk_put_text },
	{ "_sdk_start_voice", "(J)I", (void*)com_rokid_speech_Speech__sdk_start_voice },
	{ "_sdk_put_voice", "(JI[BII)V", (void*)com_rokid_speech_Speech__sdk_put_voice },
	{ "_sdk_end_voice", "(JI)V", (void*)com_rokid_speech_Speech__sdk_end_voice },
	{ "_sdk_cancel", "(JI)V", (void*)com_rokid_speech_Speech__sdk_cancel },
	{ "_sdk_config", "(JLjava/lang/String;Ljava/lang/String;)V", (void*)com_rokid_speech_Speech__sdk_config },
	{ "_sdk_poll", "(J)Lcom/rokid/speech/SpeechResult;", (void*)com_rokid_speech_Speech__sdk_poll },
};

int register_com_rokid_speech_Speech(JNIEnv* env) {
	const char* kclass = "com/rokid/speech/Speech";
	jclass target = env->FindClass(kclass);
	if (target == NULL) {
		Log::e("find class for %s failed", kclass);
		return -1;
	}

	const char* rclass_name = "com/rokid/speech/SpeechResult";
	jclass rclass = env->FindClass(rclass_name);
	if (rclass == NULL) {
		Log::e("find class for %s failed", rclass_name);
		return -1;
	}
	speech_result_class_ = (jclass)env->NewGlobalRef(rclass);
	prepare_speech_result_info(env, speech_result_class_);

	return jniRegisterNativeMethods(env, kclass, _nmethods, NELEM(_nmethods));
}

} // namespace speech
} // namespace rokid

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env;

	// store a global java vm pointer
	// for tts java callback
	rokid::speech::vm_ = vm;

	if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
		rokid::speech::Log::e("%s: JNI_OnLoad failed", "RokidTts");
		return -1;
	}
	rokid::speech::register_com_rokid_speech_Speech(env);
	return JNI_VERSION_1_4;
}
