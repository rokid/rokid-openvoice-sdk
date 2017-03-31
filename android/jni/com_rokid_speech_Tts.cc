#include <jni.h>
#include "log.h"
#include "tts.h"
#include "JNIHelp.h"

namespace rokid {
namespace speech {

static const char* tag_ = "tts.jni";
static JavaVM* vm_ = NULL;

class JniTtsCallback : public TtsCallback {
public:
	enum CallbackMethodIndex {
		START = 0,
		TEXT,
		VOICE,
		STOP,
		COMPLETE,
		ERROR,
		ERASE_CB,
		CALLBACK_METHOD_NUM
	};

	JniTtsCallback(jobject tts, jobject cb) {
		java_tts = tts;
		java_callback = cb;
	}

	void onStart(int id) {
		JNIEnv* env;
		vm_->AttachCurrentThread(&env, NULL);
		env->CallVoidMethod(java_callback, cb_methods[START], id);
		vm_->DetachCurrentThread();
	}

	void onText(int id, const char* text) {
		JNIEnv* env;
		vm_->AttachCurrentThread(&env, NULL);
		jstring s = env->NewStringUTF(text);
		env->CallVoidMethod(java_callback, cb_methods[TEXT], id, s);
		vm_->DetachCurrentThread();
	}

	void onVoice(int id, const unsigned char* data, int length) {
		JNIEnv* env;
		vm_->AttachCurrentThread(&env, NULL);

		Log::d(tag_, "onVoice %d %p:%d", id, data, length);
		jbyteArray ba = NULL;
		if (length > 0) {
			ba = env->NewByteArray(length);
			jbyte* tmp = env->GetByteArrayElements(ba, NULL);
			Log::d(tag_, "sizeof(jbyte) = %d", sizeof(jbyte));
			memcpy(tmp, data, length);
			env->ReleaseByteArrayElements(ba, tmp, JNI_COMMIT);
		}

		// debug
		// jbyte* t = env->GetByteArrayElements(ba, NULL);
		// Log::d("speech.debug", "==============================================");
		// printHexData((uint8_t*)t, length);
		// env->ReleaseByteArrayElements(ba, t, JNI_ABORT);

		Log::d(tag_, "onVoice: this = %p, cb = %p, cb_method = %p", this, java_callback, cb_methods[VOICE]);
		env->CallVoidMethod(java_callback, cb_methods[VOICE], id, ba);

		vm_->DetachCurrentThread();
	}

	void onStop(int id) {
		JNIEnv* env;
		vm_->AttachCurrentThread(&env, NULL);
		env->CallVoidMethod(java_callback, cb_methods[STOP], id);
		release(env);
		vm_->DetachCurrentThread();
	}

	void onComplete(int id) {
		JNIEnv* env;
		vm_->AttachCurrentThread(&env, NULL);
		env->CallVoidMethod(java_callback, cb_methods[COMPLETE], id);
		release(env);
		vm_->DetachCurrentThread();
	}

	void onError(int id, int err) {
		JNIEnv* env;
		vm_->AttachCurrentThread(&env, NULL);
		env->CallVoidMethod(java_callback, cb_methods[ERROR], id, err);
		release(env);
		vm_->DetachCurrentThread();
	}

	static void prepare_methods(JNIEnv* env, jobject tts, jobject cb) {
		if (cb_methods == NULL) {
			cb_methods = new jmethodID[CALLBACK_METHOD_NUM];
			jclass cb_class = env->GetObjectClass(cb);
			cb_methods[START] = env->GetMethodID(cb_class, "onStart", "(I)V");
			cb_methods[TEXT] = env->GetMethodID(cb_class, "onText", "(ILjava/lang/String;)V");
			cb_methods[VOICE] = env->GetMethodID(cb_class, "onVoice", "(I[B)V");
			cb_methods[STOP] = env->GetMethodID(cb_class, "onStop", "(I)V");
			cb_methods[COMPLETE] = env->GetMethodID(cb_class, "onComplete", "(I)V");
			cb_methods[ERROR] = env->GetMethodID(cb_class, "onError", "(II)V");

			jclass tts_class = env->GetObjectClass(tts);
			cb_methods[ERASE_CB] = env->GetMethodID(tts_class, "eraseCallback", "(J)V");
		}
	}

	static void release_methods() {
		if (cb_methods) {
			delete cb_methods;
			cb_methods = NULL;
		}
	}

private:
	void release(JNIEnv* env) {
		// erase java callback object
		env->CallVoidMethod(java_tts, cb_methods[ERASE_CB], reinterpret_cast<jlong>(this));
		env->DeleteGlobalRef(java_callback);
		env->DeleteGlobalRef(java_tts);
	}

private:
	static jmethodID* cb_methods;

	jobject java_tts;
	jobject java_callback;
};

jmethodID* JniTtsCallback::cb_methods = NULL;

static int tts_ref_count_ = 0;

static jlong com_rokid_speech_Tts__sdk_create(JNIEnv *env, jobject thiz) {
	return (jlong)(new Tts());
}

static void com_rokid_speech_Tts__sdk_delete(JNIEnv *env, jobject thiz, jlong ttsl) {
	Tts* tts = reinterpret_cast<Tts*>(ttsl);
	if (tts)
		delete tts;
}

static jboolean com_rokid_speech_Tts__sdk_prepare(JNIEnv *env, jobject thiz, jlong ttsl) {
	Tts* tts = reinterpret_cast<Tts*>(ttsl);
	if (tts == NULL)
		return false;
	if (!tts->prepare()) {
		Log::d(tag_, "prepare failed");
		return false;
	}
	++tts_ref_count_;
	return true;
}

static void com_rokid_speech_Tts__sdk_release(JNIEnv *env, jobject thiz, jlong ttsl) {
	Tts* tts = reinterpret_cast<Tts*>(ttsl);
	if (tts == NULL)
		return;
	tts->release();
	--tts_ref_count_;
	if (tts_ref_count_ == 0)
		JniTtsCallback::release_methods();
}

static jlong com_rokid_speech_Tts__sdk_create_callback(JNIEnv *env, jobject thiz, jobject jcb) {
	if (jcb == NULL)
		return 0;
	thiz = env->NewGlobalRef(thiz);
	jcb = env->NewGlobalRef(jcb);
	JniTtsCallback::prepare_methods(env, thiz, jcb);
	return (jlong)(new JniTtsCallback(thiz, jcb));
}

static jint com_rokid_speech_Tts__sdk_speak(JNIEnv *env, jobject thiz, jstring str, jlong cb, jlong ttsl) {
	Tts* tts = reinterpret_cast<Tts*>(ttsl);
	JniTtsCallback* jnicb = reinterpret_cast<JniTtsCallback*>(cb);
	if (tts == NULL || jnicb == NULL)
		return 0;
	const char* content = env->GetStringUTFChars(str, NULL);
	jint id = tts->speak(content, jnicb);
	env->ReleaseStringUTFChars(str, content);
	return id;
}

static void com_rokid_speech_Tts__sdk_stop(JNIEnv *env, jobject thiz, jint id, jlong ttsl) {
	Tts* tts = reinterpret_cast<Tts*>(ttsl);
	if (tts == NULL)
		return;
	tts->stop(id);
}

static void com_rokid_speech_Tts__sdk_config(JNIEnv *env, jobject thiz, jstring key, jstring value, jlong ttsl) {
	Tts* tts = reinterpret_cast<Tts*>(ttsl);
	if (tts == NULL)
		return;
	const char* keystr = env->GetStringUTFChars(key, NULL);
	const char* valstr = env->GetStringUTFChars(value, NULL);
	tts->config(keystr, valstr);
	env->ReleaseStringUTFChars(key, keystr);
	env->ReleaseStringUTFChars(value, valstr);
}

static JNINativeMethod _nmethods[] = {
	{ "_sdk_create", "()J", (void*)com_rokid_speech_Tts__sdk_create },
	{ "_sdk_delete", "(J)V", (void*)com_rokid_speech_Tts__sdk_delete },
	{ "_sdk_prepare", "(J)Z", (void*)com_rokid_speech_Tts__sdk_prepare },
	{ "_sdk_release", "(J)V", (void*)com_rokid_speech_Tts__sdk_release },
	{ "_sdk_create_callback", "(Lcom/rokid/speech/TtsCallback;)J", (void*)com_rokid_speech_Tts__sdk_create_callback },
	{ "_sdk_speak", "(Ljava/lang/String;JJ)I", (void*)com_rokid_speech_Tts__sdk_speak },
	{ "_sdk_stop", "(IJ)V", (void*)com_rokid_speech_Tts__sdk_stop },
	{ "_sdk_config", "(Ljava/lang/String;Ljava/lang/String;J)V", (void*)com_rokid_speech_Tts__sdk_config },
};

int register_com_rokid_speech_Tts(JNIEnv* env) {
	const char* kclass = "com/rokid/speech/Tts";
	jclass target = env->FindClass(kclass);
	if (target == NULL) {
		Log::e("find class for %s failed", kclass);
		return -1;
	}
	return jniRegisterNativeMethods(env, kclass, _nmethods, NELEM(_nmethods));
}

// implement in com_rokid_speech_Opus.cc
int register_com_rokid_speech_Opus(JNIEnv* env);

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
	rokid::speech::register_com_rokid_speech_Tts(env);
	rokid::speech::register_com_rokid_speech_Opus(env);
	return JNI_VERSION_1_4;
}
