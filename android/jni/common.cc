#include <jni.h>
#include "JNIHelp.h"
#include "log.h"
#include "speech_types.h"

namespace rokid {
namespace speech {

JavaVM* vm_ = NULL;
int register_com_rokid_speech_Tts(JNIEnv* env);
int register_com_rokid_speech_TtsOptions(JNIEnv* env);
int register_com_rokid_speech_Speech(JNIEnv* env);
int register_com_rokid_speech_SpeechOptions(JNIEnv* env);

void jobj_to_prepare_opts(JNIEnv* env, jobject obj, PrepareOptions& opts) {
	jclass cls = env->GetObjectClass(obj);

	jfieldID fld = env->GetFieldID(cls, "host", "Ljava/lang/String;");
	jstring sv = (jstring)env->GetObjectField(obj, fld);
	const char* str;
	if (sv != NULL) {
		str = env->GetStringUTFChars(sv, NULL);
		if (strlen(str) > 0)
			opts.host = str;
		env->ReleaseStringUTFChars(sv, str);
	}

	fld = env->GetFieldID(cls, "port", "I");
	opts.port = env->GetIntField(obj, fld);

	fld = env->GetFieldID(cls, "branch", "Ljava/lang/String;");
	sv = (jstring)env->GetObjectField(obj, fld);
	if (sv != NULL) {
		str = env->GetStringUTFChars(sv, NULL);
		if (strlen(str) > 0)
			opts.branch = str;
		env->ReleaseStringUTFChars(sv, str);
	}

	fld = env->GetFieldID(cls, "key", "Ljava/lang/String;");
	sv = (jstring)env->GetObjectField(obj, fld);
	if (sv != NULL) {
		str = env->GetStringUTFChars(sv, NULL);
		opts.key = str;
		env->ReleaseStringUTFChars(sv, str);
	}

	fld = env->GetFieldID(cls, "device_type_id", "Ljava/lang/String;");
	sv = (jstring)env->GetObjectField(obj, fld);
	if (sv != NULL) {
		str = env->GetStringUTFChars(sv, NULL);
		opts.device_type_id = str;
		env->ReleaseStringUTFChars(sv, str);
	}

	fld = env->GetFieldID(cls, "secret", "Ljava/lang/String;");
	sv = (jstring)env->GetObjectField(obj, fld);
	if (sv != NULL) {
		str = env->GetStringUTFChars(sv, NULL);
		opts.secret = str;
		env->ReleaseStringUTFChars(sv, str);
	}

	fld = env->GetFieldID(cls, "device_id", "Ljava/lang/String;");
	sv = (jstring)env->GetObjectField(obj, fld);
	if (sv != NULL) {
		str = env->GetStringUTFChars(sv, NULL);
		opts.device_id = str;
		env->ReleaseStringUTFChars(sv, str);
	}

	fld = env->GetFieldID(cls, "reconn_interval", "I");
	opts.reconn_interval = env->GetIntField(obj, fld);
	fld = env->GetFieldID(cls, "ping_interval", "I");
	opts.ping_interval = env->GetIntField(obj, fld);
	fld = env->GetFieldID(cls, "no_resp_timeout", "I");
	opts.no_resp_timeout = env->GetIntField(obj, fld);
}

} // namespace speech
} // namespace rokid

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env;

	// store a global java vm pointer
	// for speech java callback
	rokid::speech::vm_ = vm;

	if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
		rokid::speech::Log::e("%s: JNI_OnLoad failed", "RokidSpeech");
		return -1;
	}
	rokid::speech::register_com_rokid_speech_Tts(env);
	rokid::speech::register_com_rokid_speech_TtsOptions(env);
	rokid::speech::register_com_rokid_speech_Speech(env);
	rokid::speech::register_com_rokid_speech_SpeechOptions(env);
	return JNI_VERSION_1_4;
}
