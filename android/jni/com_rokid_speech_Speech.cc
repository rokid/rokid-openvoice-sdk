#include <jni.h>
#include <thread>
#include <assert.h>
#include "log.h"
#include "speech.h"
#include "JNIHelp.h"

using std::thread;
using std::shared_ptr;

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
	RES_EXTRA,

	RES_FIELD_NUM
};

enum VoiceOptionsFieldAlias {
	OPTS_STACK,
	OPTS_VOICE_TRIGGER,
	OPTS_TRIGGER_START,
	OPTS_TRIGGER_LENGTH,
	OPTS_SKILL_OPTIONS,
	OPTS_VOICE_EXTRA,

	OPTS_FIELD_NUM
};

typedef struct {
	jmethodID handle_callback;
	jfieldID result_fields[RES_FIELD_NUM];
	jclass result_class;
	jfieldID voice_options_fields[OPTS_FIELD_NUM];
	jclass voice_options_class;
} SpeechNativeConstants;

static SpeechNativeConstants constants_;

class SpeechPollThread {
public:
	void start(shared_ptr<Speech> speech, jobject speech_obj) {
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
		if (result.extra.length() > 0) {
			strobj = env_->NewStringUTF(result.extra.c_str());
			env_->SetObjectField(res_obj, constants_.result_fields[RES_EXTRA], strobj);
		}
		return res_obj;
	}

private:
	thread* thread_;
	shared_ptr<Speech> speech_;
	jobject speech_obj_;
	JNIEnv* env_;
};

typedef struct {
	shared_ptr<Speech> speech;
	SpeechPollThread* poll_thread;
} SpeechNativeInfo;

typedef struct {
	shared_ptr<SpeechOptions> options;
} SpeechOptionsNativeInfo;

// implement in common.cc
extern void jobj_to_prepare_opts(JNIEnv* env, jobject obj, PrepareOptions& opts);

static void com_rokid_speech_Speech__sdk_init(JNIEnv *env,
		jobject thiz, jclass speech_cls, jclass res_cls,
		jclass options_cls) {
	assert(vm_);
	constants_.handle_callback = env->GetMethodID(
			speech_cls, "handle_callback",
			"(Lcom/rokid/speech/Speech$SpeechResult;)V");
	constants_.result_fields[RES_ID] = env->GetFieldID(res_cls,
			"id", "I");
	constants_.result_fields[RES_TYPE] = env->GetFieldID(res_cls,
			"type", "I");
	constants_.result_fields[RES_ERROR] = env->GetFieldID(res_cls,
			"err", "I");
	constants_.result_fields[RES_ASR] = env->GetFieldID(res_cls,
			"asr", "Ljava/lang/String;");
	constants_.result_fields[RES_NLP] = env->GetFieldID(res_cls,
			"nlp", "Ljava/lang/String;");
	constants_.result_fields[RES_ACTION] = env->GetFieldID(res_cls,
			"action", "Ljava/lang/String;");
	constants_.result_fields[RES_EXTRA] = env->GetFieldID(res_cls,
			"extra", "Ljava/lang/String;");
	constants_.result_class = (jclass)env->NewGlobalRef(res_cls);
	constants_.voice_options_fields[OPTS_STACK] = env->GetFieldID(
			options_cls, "stack", "Ljava/lang/String;");
	constants_.voice_options_fields[OPTS_VOICE_TRIGGER] = env->GetFieldID(
			options_cls, "voice_trigger", "Ljava/lang/String;");
	constants_.voice_options_fields[OPTS_TRIGGER_START] = env->GetFieldID(
			options_cls, "trigger_start", "I");
	constants_.voice_options_fields[OPTS_TRIGGER_LENGTH] = env->GetFieldID(
			options_cls, "trigger_length", "I");
	constants_.voice_options_fields[OPTS_SKILL_OPTIONS] = env->GetFieldID(
			options_cls, "skill_options", "Ljava/lang/String;");
	constants_.voice_options_fields[OPTS_VOICE_EXTRA] = env->GetFieldID(
			options_cls, "voice_extra", "Ljava/lang/String;");
	constants_.voice_options_class = (jclass)env->NewGlobalRef(options_cls);
}

static jlong com_rokid_speech_Speech__sdk_create(JNIEnv *env, jobject thiz) {
	SpeechNativeInfo* r = new SpeechNativeInfo();
	memset(r, 0, sizeof(*r));
	r->speech = Speech::new_instance();
	return (jlong)r;
}

static void com_rokid_speech_Speech__sdk_delete(JNIEnv *env, jobject thiz, jlong speechl) {
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	if (p)
		delete p;
}

static jboolean com_rokid_speech_Speech__sdk_prepare(JNIEnv *env, jobject thiz, jlong speechl, jobject opt) {
	if (opt == NULL)
		return false;
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	assert(p);
	PrepareOptions popts;
	jobj_to_prepare_opts(env, opt, popts);
	if (!p->speech->prepare(popts)) {
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

static void jobj_to_voice_options(JNIEnv* env, jobject obj, VoiceOptions& opts) {
	jstring sv = (jstring)env->GetObjectField(obj, constants_.voice_options_fields[OPTS_STACK]);
	const char* str;
	if (sv != NULL) {
		str = env->GetStringUTFChars(sv, NULL);
		opts.stack = str;
		env->ReleaseStringUTFChars(sv, str);
	}

	sv = (jstring)env->GetObjectField(obj, constants_.voice_options_fields[OPTS_VOICE_TRIGGER]);
	if (sv != NULL) {
		str = env->GetStringUTFChars(sv, NULL);
		opts.voice_trigger = str;
		env->ReleaseStringUTFChars(sv, str);
	}

	opts.trigger_start = env->GetIntField(obj, constants_.voice_options_fields[OPTS_TRIGGER_START]);
	opts.trigger_length = env->GetIntField(obj, constants_.voice_options_fields[OPTS_TRIGGER_LENGTH]);

	sv = (jstring)env->GetObjectField(obj, constants_.voice_options_fields[OPTS_SKILL_OPTIONS]);
	if (sv != NULL) {
		str = env->GetStringUTFChars(sv, NULL);
		opts.skill_options = str;
		env->ReleaseStringUTFChars(sv, str);
	}

	sv = (jstring)env->GetObjectField(obj, constants_.voice_options_fields[OPTS_VOICE_EXTRA]);
	if (sv != NULL) {
		str = env->GetStringUTFChars(sv, NULL);
		opts.voice_extra = str;
		env->ReleaseStringUTFChars(sv, str);
	}

}

static jint com_rokid_speech_Speech__sdk_put_text(JNIEnv *env, jobject thiz, jlong speechl, jstring str, jobject opts) {
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	assert(p);
	const char* content = env->GetStringUTFChars(str, NULL);
	jint id;
	if (opts != NULL) {
		VoiceOptions vopts;
		jobj_to_voice_options(env, opts, vopts);
		id = p->speech->put_text(content, &vopts);
	} else
		id = p->speech->put_text(content);
	env->ReleaseStringUTFChars(str, content);
	return id;
}

static shared_ptr<SpeechOptions> jobj_to_speech_options(JNIEnv* env, jobject obj) {
	jclass cls = env->GetObjectClass(obj);
	jfieldID fld = env->GetFieldID(cls, "_native_options", "J");
	jlong fv = env->GetLongField(obj, fld);
	return reinterpret_cast<SpeechOptionsNativeInfo*>(fv)->options;
}

static jint com_rokid_speech_Speech__sdk_start_voice(JNIEnv *env,
		jobject thiz, jlong speechl, jobject opts) {
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	assert(p);
	if (opts != NULL) {
		VoiceOptions vopts;
		jobj_to_voice_options(env, opts, vopts);
		return p->speech->start_voice(&vopts);
	}
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

static void com_rokid_speech_Speech__sdk_config(JNIEnv *env, jobject thiz, jlong speechl, jobject opt) {
	SpeechNativeInfo* p = reinterpret_cast<SpeechNativeInfo*>(speechl);
	assert(p);
	if (opt != NULL) {
		shared_ptr<SpeechOptions> sopts = jobj_to_speech_options(env, opt);
		p->speech->config(sopts);
	}
}

static jlong com_rokid_speech_SpeechOptions_native_new_options(
		JNIEnv* env, jobject thiz) {
	SpeechOptionsNativeInfo* p = new SpeechOptionsNativeInfo();
	p->options = SpeechOptions::new_instance();
	return (jlong)p;
}

static void com_rokid_speech_SpeechOptions_native_release(
		JNIEnv* env, jobject thiz, jlong optl) {
	SpeechOptionsNativeInfo* p =
		reinterpret_cast<SpeechOptionsNativeInfo*>(optl);
	assert(p);
	p->options.reset();
	delete p;
}

static void com_rokid_speech_SpeechOptions_native_set_lang(JNIEnv* env,
		jobject thiz, jlong optl, jstring v) {
	SpeechOptionsNativeInfo* p =
		reinterpret_cast<SpeechOptionsNativeInfo*>(optl);
	const char* sv;
	Lang lang;

	assert(p);
	sv = env->GetStringUTFChars(v, NULL);
	if (strcmp(sv, "en") == 0)
		lang = Lang::EN;
	else
		lang = Lang::ZH;
	p->options->set_lang(lang);
	env->ReleaseStringUTFChars(v, sv);
}

static void com_rokid_speech_SpeechOptions_native_set_codec(JNIEnv* env,
		jobject thiz, jlong optl, jstring v) {
	SpeechOptionsNativeInfo* p =
		reinterpret_cast<SpeechOptionsNativeInfo*>(optl);
	const char* sv;
	Codec codec;

	assert(p);
	sv = env->GetStringUTFChars(v, NULL);
	if (strcmp(sv, "opu") == 0)
		codec = Codec::OPU;
	else
		codec = Codec::PCM;
	p->options->set_codec(codec);
	env->ReleaseStringUTFChars(v, sv);
}

static void com_rokid_speech_SpeechOptions_native_set_vad_mode(JNIEnv* env,
		jobject thiz, jlong optl, jstring v) {
	SpeechOptionsNativeInfo* p =
		reinterpret_cast<SpeechOptionsNativeInfo*>(optl);
	const char* sv;
	VadMode mode;

	assert(p);
	sv = env->GetStringUTFChars(v, NULL);
	if (strcmp(sv, "cloud") == 0)
		mode = VadMode::CLOUD;
	else
		mode = VadMode::LOCAL;
	p->options->set_vad_mode(mode);
	env->ReleaseStringUTFChars(v, sv);
}

static void com_rokid_speech_SpeechOptions_native_set_no_nlp(JNIEnv* env,
		jobject thiz, jlong optl, jboolean v) {
	SpeechOptionsNativeInfo* p =
		reinterpret_cast<SpeechOptionsNativeInfo*>(optl);

	assert(p);
	p->options->set_no_nlp(v);
}

static void com_rokid_speech_SpeechOptions_native_set_no_intermediate_asr(JNIEnv* env,
		jobject thiz, jlong optl, jboolean v) {
	SpeechOptionsNativeInfo* p =
		reinterpret_cast<SpeechOptionsNativeInfo*>(optl);

	assert(p);
	p->options->set_no_intermediate_asr(v);
}

static JNINativeMethod _nmethods[] = {
	{ "_sdk_init", "(Ljava/lang/Class;Ljava/lang/Class;Ljava/lang/Class;)V", (void*)com_rokid_speech_Speech__sdk_init },
	{ "_sdk_create", "()J", (void*)com_rokid_speech_Speech__sdk_create },
	{ "_sdk_delete", "(J)V", (void*)com_rokid_speech_Speech__sdk_delete },
	{ "_sdk_prepare", "(JLcom/rokid/speech/PrepareOptions;)Z", (void*)com_rokid_speech_Speech__sdk_prepare },
	{ "_sdk_release", "(J)V", (void*)com_rokid_speech_Speech__sdk_release },
	{ "_sdk_put_text", "(JLjava/lang/String;Lcom/rokid/speech/Speech$VoiceOptions;)I", (void*)com_rokid_speech_Speech__sdk_put_text },
	{ "_sdk_start_voice", "(JLcom/rokid/speech/Speech$VoiceOptions;)I", (void*)com_rokid_speech_Speech__sdk_start_voice },
	{ "_sdk_put_voice", "(JI[BII)V", (void*)com_rokid_speech_Speech__sdk_put_voice },
	{ "_sdk_end_voice", "(JI)V", (void*)com_rokid_speech_Speech__sdk_end_voice },
	{ "_sdk_cancel", "(JI)V", (void*)com_rokid_speech_Speech__sdk_cancel },
	{ "_sdk_config", "(JLcom/rokid/speech/SpeechOptions;)V", (void*)com_rokid_speech_Speech__sdk_config },
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

static JNINativeMethod _options_nmethods[] = {
	{ "native_new_options", "()J", (void*)com_rokid_speech_SpeechOptions_native_new_options },
	{ "native_release", "(J)V", (void*)com_rokid_speech_SpeechOptions_native_release },
	{ "native_set_lang", "(JLjava/lang/String;)V", (void*)com_rokid_speech_SpeechOptions_native_set_lang },
	{ "native_set_codec", "(JLjava/lang/String;)V", (void*)com_rokid_speech_SpeechOptions_native_set_codec },
	{ "native_set_vad_mode", "(JLjava/lang/String;)V", (void*)com_rokid_speech_SpeechOptions_native_set_vad_mode },
	{ "native_set_no_nlp", "(JZ)V", (void*)com_rokid_speech_SpeechOptions_native_set_no_nlp },
	{ "native_set_no_intermediate_asr", "(JZ)V", (void*)com_rokid_speech_SpeechOptions_native_set_no_intermediate_asr },
};

int register_com_rokid_speech_SpeechOptions(JNIEnv* env) {
	const char* kclass = "com/rokid/speech/SpeechOptions";
	jclass target = env->FindClass(kclass);
	if (target == NULL) {
		Log::e("find class for %s failed", kclass);
		return -1;
	}
	return jniRegisterNativeMethods(env, kclass, _options_nmethods, NELEM(_options_nmethods));
}

} // namespace speech
} // namespace rokid
