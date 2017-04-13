#include <jni.h>
#include <thread>
#include "log.h"
#include "tts.h"
#include "JNIHelp.h"

using std::thread;

namespace rokid {
namespace speech {

static const char* tag_ = "tts.jni";
static JavaVM* vm_ = NULL;

enum TtsResultFieldAlias {
	RES_ID,
	RES_TYPE,
	RES_ERROR,
	RES_VOICE,

	RES_FIELD_NUM
};

typedef struct {
	jmethodID handle_callback;
	jfieldID result_fields[RES_FIELD_NUM];
	jclass result_class;
} TtsNativeConstants;

static TtsNativeConstants constants_;

class TtsPollThread {
public:
	void start(Tts* tts, jobject tts_obj) {
		tts_ = tts;
		tts_obj_ = tts_obj;
		thread_ = new thread([=] { run(); });
	}

	void close() {
		env_->DeleteGlobalRef(tts_obj_);
		thread_->join();
		delete thread_;
	}

protected:
	void run() {
		vm_->AttachCurrentThread(&env_, NULL);
		do_poll();
		vm_->DetachCurrentThread();
	}

	void do_poll() {
		TtsResult result;
		int32_t r;
		jobject res_obj;
		while (true) {
			if (!tts_->poll(result))
				break;
			env_->PushLocalFrame(32);
			res_obj = generate_result_object(result);
			env_->CallVoidMethod(tts_obj_, constants_.handle_callback, res_obj);
			env_->PopLocalFrame(NULL);
		}
	}

private:
	jobject generate_result_object(TtsResult& result) {
		jobject res_obj = env_->AllocObject(constants_.result_class);
		env_->SetIntField(res_obj, constants_.result_fields[RES_ID], result.id);
		env_->SetIntField(res_obj, constants_.result_fields[RES_TYPE], result.type);
		env_->SetIntField(res_obj, constants_.result_fields[RES_ERROR], result.err);
		jbyteArray voice_obj = 0;
		if (result.voice.get() && result.voice->length() > 0) {
			voice_obj = env_->NewByteArray(result.voice->length());
			jbyte* tmp = env_->GetByteArrayElements(voice_obj, NULL);
			memcpy(tmp, result.voice->data(), result.voice->length());
			env_->ReleaseByteArrayElements(voice_obj, tmp, JNI_COMMIT);
		}
		env_->SetObjectField(res_obj, constants_.result_fields[RES_VOICE], voice_obj);
		return res_obj;
	}

private:
	thread* thread_;
	Tts* tts_;
	jobject tts_obj_;
	JNIEnv* env_;
};

typedef struct {
	Tts* tts;
	TtsPollThread* poll_thread;
} TtsNativeInfo;

static void com_rokid_speech_Tts__sdk_init(JNIEnv *env, jobject thiz, jclass tts_cls, jclass res_cls) {
	assert(vm_);
	constants_.handle_callback = env->GetMethodID(
			tts_cls, "handle_callback", "(Lcom/rokid/speech/Tts$TtsResult;)V");
	constants_.result_fields[RES_ID] = env->GetFieldID(res_cls, "id", "I");
	constants_.result_fields[RES_TYPE] = env->GetFieldID(res_cls, "type", "I");
	constants_.result_fields[RES_ERROR] = env->GetFieldID(res_cls, "err", "I");
	constants_.result_fields[RES_VOICE] = env->GetFieldID(res_cls, "voice", "[B");
	constants_.result_class = (jclass)env->NewGlobalRef(res_cls);
}

static jlong com_rokid_speech_Tts__sdk_create(JNIEnv *env, jobject thiz) {
	TtsNativeInfo* r = new TtsNativeInfo();
	memset(r, 0, sizeof(*r));
	r->tts = new_tts();
	return (jlong)r;
}

static void com_rokid_speech_Tts__sdk_delete(JNIEnv *env, jobject thiz, jlong ttsl) {
	TtsNativeInfo* p = reinterpret_cast<TtsNativeInfo*>(ttsl);
	if (p) {
		delete_tts(p->tts);
		delete p;
	}
}

static jboolean com_rokid_speech_Tts__sdk_prepare(JNIEnv *env, jobject thiz, jlong ttsl) {
	TtsNativeInfo* p = reinterpret_cast<TtsNativeInfo*>(ttsl);
	assert(p);
	if (!p->tts->prepare()) {
		Log::d(tag_, "prepare failed");
		return false;
	}
	p->poll_thread = new TtsPollThread();
	p->poll_thread->start(p->tts, env->NewGlobalRef(thiz));
	return true;
}

static void com_rokid_speech_Tts__sdk_release(JNIEnv *env, jobject thiz, jlong ttsl) {
	TtsNativeInfo* p = reinterpret_cast<TtsNativeInfo*>(ttsl);
	assert(p);
	p->tts->release();
	if (p->poll_thread) {
		p->poll_thread->close();
		delete p->poll_thread;
		p->poll_thread = NULL;
	}
}

static jint com_rokid_speech_Tts__sdk_speak(JNIEnv *env, jobject thiz, jlong ttsl, jstring str) {
	TtsNativeInfo* p = reinterpret_cast<TtsNativeInfo*>(ttsl);
	assert(p);
	const char* content = env->GetStringUTFChars(str, NULL);
	jint id = p->tts->speak(content);
	env->ReleaseStringUTFChars(str, content);
	return id;
}

static void com_rokid_speech_Tts__sdk_cancel(JNIEnv *env, jobject thiz, jlong ttsl, jint id) {
	TtsNativeInfo* p = reinterpret_cast<TtsNativeInfo*>(ttsl);
	assert(p);
	p->tts->cancel(id);
}

static void com_rokid_speech_Tts__sdk_config(JNIEnv *env, jobject thiz, jlong ttsl, jstring key, jstring value) {
	TtsNativeInfo* p = reinterpret_cast<TtsNativeInfo*>(ttsl);
	assert(p);
	const char* keystr = env->GetStringUTFChars(key, NULL);
	const char* valstr = env->GetStringUTFChars(value, NULL);
	p->tts->config(keystr, valstr);
	env->ReleaseStringUTFChars(key, keystr);
	env->ReleaseStringUTFChars(value, valstr);
}

static JNINativeMethod _nmethods[] = {
	{ "_sdk_init", "(Ljava/lang/Class;Ljava/lang/Class;)V", (void*)com_rokid_speech_Tts__sdk_init },
	{ "_sdk_create", "()J", (void*)com_rokid_speech_Tts__sdk_create },
	{ "_sdk_delete", "(J)V", (void*)com_rokid_speech_Tts__sdk_delete },
	{ "_sdk_prepare", "(J)Z", (void*)com_rokid_speech_Tts__sdk_prepare },
	{ "_sdk_release", "(J)V", (void*)com_rokid_speech_Tts__sdk_release },
	{ "_sdk_speak", "(JLjava/lang/String;)I", (void*)com_rokid_speech_Tts__sdk_speak },
	{ "_sdk_cancel", "(JI)V", (void*)com_rokid_speech_Tts__sdk_cancel },
	{ "_sdk_config", "(JLjava/lang/String;Ljava/lang/String;)V", (void*)com_rokid_speech_Tts__sdk_config },
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
	return JNI_VERSION_1_4;
}
