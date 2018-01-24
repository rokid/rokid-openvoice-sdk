#include <jni.h>
#include <thread>
#include <assert.h>
#include "log.h"
#include "tts.h"
#include "JNIHelp.h"

using std::thread;
using std::shared_ptr;

namespace rokid {
namespace speech {

extern JavaVM* vm_;
static const char* tag_ = "tts.jni";

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
	void start(shared_ptr<Tts> tts, jobject tts_obj) {
		tts_ = tts;
		tts_obj_ = tts_obj;
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
		env_->DeleteGlobalRef(tts_obj_);
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
			env_->ReleaseByteArrayElements(voice_obj, tmp, 0);
		}
		env_->SetObjectField(res_obj, constants_.result_fields[RES_VOICE], voice_obj);
		return res_obj;
	}

private:
	thread* thread_;
	shared_ptr<Tts> tts_;
	jobject tts_obj_;
	JNIEnv* env_;
};

typedef struct {
	shared_ptr<Tts> tts;
	TtsPollThread* poll_thread;
} TtsNativeInfo;

// implement in common.cc
extern void jobj_to_prepare_opts(JNIEnv* env, jobject obj, PrepareOptions& opts);

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
	r->tts = Tts::new_instance();
	return (jlong)r;
}

static void com_rokid_speech_Tts__sdk_delete(JNIEnv *env, jobject thiz, jlong ttsl) {
	TtsNativeInfo* p = reinterpret_cast<TtsNativeInfo*>(ttsl);
	if (p)
		delete p;
}

static jboolean com_rokid_speech_Tts__sdk_prepare(JNIEnv *env, jobject thiz, jlong ttsl, jobject opt) {
	TtsNativeInfo* p = reinterpret_cast<TtsNativeInfo*>(ttsl);
	assert(p);
	PrepareOptions popts;
	jobj_to_prepare_opts(env, opt, popts);
	if (!p->tts->prepare(popts)) {
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

typedef struct {
	shared_ptr<TtsOptions> opts;
} TtsOptionsNativeInfo;

static shared_ptr<TtsOptions> jobj_to_tts_opts(JNIEnv* env, jobject obj) {
	jclass cls = env->GetObjectClass(obj);
	jfieldID fld = env->GetFieldID(cls, "_tts_options", "J");
	jlong fv = env->GetLongField(obj, fld);
	return reinterpret_cast<TtsOptionsNativeInfo*>(fv)->opts;
}

static void com_rokid_speech_Tts__sdk_config(JNIEnv *env, jobject thiz, jlong ttsl, jobject opt) {
	TtsNativeInfo* p = reinterpret_cast<TtsNativeInfo*>(ttsl);
	assert(p);
	shared_ptr<TtsOptions> topts = jobj_to_tts_opts(env, opt);
	p->tts->config(topts);
}

static jlong com_rokid_speech_TtsOptions_native_new_options(JNIEnv *env, jobject thiz) {
	TtsOptionsNativeInfo* native_info = new TtsOptionsNativeInfo();
	native_info->opts = TtsOptions::new_instance();
	return reinterpret_cast<jlong>(native_info);
}

static void com_rokid_speech_TtsOptions_native_release(JNIEnv *env, jobject thiz, jlong opt) {
	if (opt != 0) {
		delete reinterpret_cast<TtsOptionsNativeInfo*>(opt);
	}
}

static void com_rokid_speech_TtsOptions_native_set_codec(JNIEnv *env, jobject thiz, jlong opt, jstring v) {
	TtsOptionsNativeInfo* native_info = reinterpret_cast<TtsOptionsNativeInfo*>(opt);
	const char* sv = env->GetStringUTFChars(v, NULL);
	Codec codec;
	if (strcmp(sv, "opu2") == 0)
		codec = Codec::OPU2;
	else if (strcmp(sv, "mp3") == 0)
		codec = Codec::MP3;
	else
		codec = Codec::PCM;
	native_info->opts->set_codec(codec);
	env->ReleaseStringUTFChars(v, sv);
}

static void com_rokid_speech_TtsOptions_native_set_declaimer(JNIEnv *env, jobject thiz, jlong opt, jstring v) {
	TtsOptionsNativeInfo* native_info = reinterpret_cast<TtsOptionsNativeInfo*>(opt);
	const char* sv = env->GetStringUTFChars(v, NULL);
	native_info->opts->set_declaimer(sv);
	env->ReleaseStringUTFChars(v, sv);
}

static void com_rokid_speech_TtsOptions_native_set_samplerate(JNIEnv *env, jobject thiz, jlong opt, jint v) {
	TtsOptionsNativeInfo* native_info = reinterpret_cast<TtsOptionsNativeInfo*>(opt);
	native_info->opts->set_samplerate(v);
}

static JNINativeMethod _tts_nmethods[] = {
	{ "_sdk_init", "(Ljava/lang/Class;Ljava/lang/Class;)V", (void*)com_rokid_speech_Tts__sdk_init },
	{ "_sdk_create", "()J", (void*)com_rokid_speech_Tts__sdk_create },
	{ "_sdk_delete", "(J)V", (void*)com_rokid_speech_Tts__sdk_delete },
	{ "_sdk_prepare", "(JLcom/rokid/speech/PrepareOptions;)Z", (void*)com_rokid_speech_Tts__sdk_prepare },
	{ "_sdk_release", "(J)V", (void*)com_rokid_speech_Tts__sdk_release },
	{ "_sdk_speak", "(JLjava/lang/String;)I", (void*)com_rokid_speech_Tts__sdk_speak },
	{ "_sdk_cancel", "(JI)V", (void*)com_rokid_speech_Tts__sdk_cancel },
	{ "_sdk_config", "(JLcom/rokid/speech/TtsOptions;)V", (void*)com_rokid_speech_Tts__sdk_config },
};

int register_com_rokid_speech_Tts(JNIEnv* env) {
	const char* kclass = "com/rokid/speech/Tts";
	jclass target = env->FindClass(kclass);
	if (target == NULL) {
		Log::e("find class for %s failed", kclass);
		return -1;
	}
	return jniRegisterNativeMethods(env, kclass, _tts_nmethods, NELEM(_tts_nmethods));
}

static JNINativeMethod _tts_opt_nmethods[] = {
	{ "native_new_options", "()J", (void*)com_rokid_speech_TtsOptions_native_new_options },
	{ "native_release", "(J)V", (void*)com_rokid_speech_TtsOptions_native_release },
	{ "native_set_codec", "(JLjava/lang/String;)V", (void*)com_rokid_speech_TtsOptions_native_set_codec },
	{ "native_set_declaimer", "(JLjava/lang/String;)V", (void*)com_rokid_speech_TtsOptions_native_set_declaimer },
	{ "native_set_samplerate", "(JI)V", (void*)com_rokid_speech_TtsOptions_native_set_samplerate },
};

int register_com_rokid_speech_TtsOptions(JNIEnv* env) {
	const char* kclass = "com/rokid/speech/TtsOptions";
	jclass target = env->FindClass(kclass);
	if (target == NULL) {
		Log::e("find class for %s failed", kclass);
		return -1;
	}
	return jniRegisterNativeMethods(env, kclass, _tts_opt_nmethods, NELEM(_tts_opt_nmethods));
}

} // namespace speech
} // namespace rokid
