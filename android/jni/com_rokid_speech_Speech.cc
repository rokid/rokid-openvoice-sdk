#include <jni.h>
#include <thread>
#include <assert.h>
#include "log.h"
#include "speech.h"
#include "JNIHelp.h"

using std::thread;

namespace rokid {
namespace speech {

extern JavaVM* vm_;
static const char* tag_ = "speech.jni";

enum SpeechResultFieldAlias {
	RES_ID,
	RES_TYPE,
	RES_ERROR,
	RES_ASR,
	RES_NLP,
	RES_ACTION,

	RES_FIELD_NUM
};

typedef struct {
	jmethodID handle_callback;
	jfieldID result_fields[RES_FIELD_NUM];
	jclass result_class;
} SpeechNativeConstants;

static SpeechNativeConstants constants_;

class SpeechPollThread {
public:
	void start(Speech* speech, jobject speech_obj) {
		speech_ = speech;
		speech_obj_ = speech_obj;
		thread_ = new thread([=] { run(); });
	}

	void close() {
		thread_->join();
		delete thread_;
	}

protected:
	void run() {
		vm_->AttachCurrentThread(&env_, NULL);
		do_poll();
		env_->DeleteGlobalRef(speech_obj_);
		vm_->DetachCurrentThread();
	}

	void do_poll() {
		SpeechResult result;
		int32_t r;
		jobject res_obj;
		while (true) {
			if (!speech_->poll(result))
				break;
			env_->PushLocalFrame(32);
			res_obj = generate_result_object(result);
			env_->CallVoidMethod(speech_obj_, constants_.handle_callback, res_obj);
			env_->PopLocalFrame(NULL);
		}
	}

private:
	jobject generate_result_object(SpeechResult& result) {
		jobject res_obj = env_->AllocObject(constants_.result_class);
		env_->SetIntField(res_obj, constants_.result_fields[RES_ID], result.id);
		env_->SetIntField(res_obj, constants_.result_fields[RES_TYPE], result.type);
		env_->SetIntField(res_obj, constants_.result_fields[RES_ERROR], result.err);
		jstring strobj;
		if (result.asr.length() > 0) {
			strobj = env_->NewStringUTF(result.asr.c_str());
			env_->SetObjectField(res_obj, constants_.result_fields[RES_ASR], strobj);
		}
		if (result.nlp.length() > 0) {
			strobj = env_->NewStringUTF(result.nlp.c_str());
			env_->SetObjectField(res_obj, constants_.result_fields[RES_NLP], strobj);
		}
		if (result.action.length() > 0) {
			strobj = env_->NewStringUTF(result.action.c_str());
			env_->SetObjectField(res_obj, constants_.result_fields[RES_ACTION], strobj);
		}
		return res_obj;
	}

private:
	thread* thread_;
	Speech* speech_;
	jobject speech_obj_;
	JNIEnv* env_;
};

typedef struct {
	Speech* speech;
	SpeechPollThread* poll_thread;
} SpeechNativeInfo;

static void com_rokid_speech_Speech__sdk_init(JNIEnv *env, jobject thiz, jclass speech_cls, jclass res_cls) {
	assert(vm_);
	constants_.handle_callback = env->GetMethodID(
			speech_cls, "handle_callback", "(Lcom/rokid/speech/Speech$SpeechResult;)V");
	constants_.result_fields[RES_ID] = env->GetFieldID(res_cls, "id", "I");
	constants_.result_fields[RES_TYPE] = env->GetFieldID(res_cls, "type", "I");
	constants_.result_fields[RES_ERROR] = env->GetFieldID(res_cls, "err", "I");
	constants_.result_fields[RES_ASR] = env->GetFieldID(res_cls, "asr", "Ljava/lang/String;");
	constants_.result_fields[RES_NLP] = env->GetFieldID(res_cls, "nlp", "Ljava/lang/String;");
	constants_.result_fields[RES_ACTION] = env->GetFieldID(res_cls, "action", "Ljava/lang/String;");
	constants_.result_class = (jclass)env->NewGlobalRef(res_cls);
}

static jlong com_rokid_speech_Speech__sdk_create(JNIEnv *env, jobject thiz) {
	SpeechNativeInfo* r = new SpeechNativeInfo();
	memset(r, 0, sizeof(*r));
	r->speech = new_speech();
	return (jlong)r;
}

static void com_rokid_speech_Speech__sdk_delete(JNIEnv *env, jobject thiz, jlong speechl) {
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	if (p) {
		delete_speech(p->speech);
		delete p;
	}
}

static jboolean com_rokid_speech_Speech__sdk_prepare(JNIEnv *env, jobject thiz, jlong speechl) {
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	assert(p);
	if (!p->speech->prepare()) {
		Log::d(tag_, "prepare failed");
		return false;
	}
	p->poll_thread = new SpeechPollThread();
	p->poll_thread->start(p->speech, env->NewGlobalRef(thiz));
	return true;
}

static void com_rokid_speech_Speech__sdk_release(JNIEnv *env, jobject thiz, jlong speechl) {
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	assert(p);
	p->speech->release();
	if (p->poll_thread) {
		p->poll_thread->close();
		delete p->poll_thread;
		p->poll_thread = NULL;
	}
}

static jint com_rokid_speech_Speech__sdk_put_text(JNIEnv *env, jobject thiz, jlong speechl, jstring str) {
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	assert(p);
	const char* content = env->GetStringUTFChars(str, NULL);
	jint id = p->speech->put_text(content);
	env->ReleaseStringUTFChars(str, content);
	return id;
}

static jint com_rokid_speech_Speech__sdk_start_voice(JNIEnv *env, jobject thiz, jlong speechl) {
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	assert(p);
	return p->speech->start_voice();
}

static void com_rokid_speech_Speech__sdk_put_voice(JNIEnv *env,
		jobject thiz, jlong speechl, jint id, jbyteArray voice, jint offset, jint length) {
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	assert(p);
	jbyte* buf = new jbyte[length];
	env->GetByteArrayRegion(voice, offset, length, buf);
	p->speech->put_voice(id, (uint8_t*)buf, length);
	delete buf;
}

static void com_rokid_speech_Speech__sdk_end_voice(JNIEnv *env,
		jobject thiz, jlong speechl, jint id) {
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	assert(p);
	p->speech->end_voice(id);
}

static void com_rokid_speech_Speech__sdk_cancel(JNIEnv *env, jobject thiz, jlong speechl, jint id) {
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	assert(p);
	p->speech->cancel(id);
}

static void com_rokid_speech_Speech__sdk_config(JNIEnv *env, jobject thiz, jlong speechl, jstring key, jstring value) {
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	assert(p);
	const char* keystr = env->GetStringUTFChars(key, NULL);
	const char* valstr = env->GetStringUTFChars(value, NULL);
	p->speech->config(keystr, valstr);
	env->ReleaseStringUTFChars(key, keystr);
	env->ReleaseStringUTFChars(value, valstr);
}

static JNINativeMethod _nmethods[] = {
	{ "_sdk_init", "(Ljava/lang/Class;Ljava/lang/Class;)V", (void*)com_rokid_speech_Speech__sdk_init },
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
};

int register_com_rokid_speech_Speech(JNIEnv* env) {
	const char* kclass = "com/rokid/speech/Speech";
	jclass target = env->FindClass(kclass);
	if (target == NULL) {
		Log::e("find class for %s failed", kclass);
		return -1;
	}
	return jniRegisterNativeMethods(env, kclass, _nmethods, NELEM(_nmethods));
}

} // namespace speech
} // namespace rokid
