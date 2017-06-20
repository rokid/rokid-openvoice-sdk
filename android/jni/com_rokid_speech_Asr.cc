#include <jni.h>
#include <thread>
#include <assert.h>
#include "log.h"
#include "asr.h"
#include "JNIHelp.h"

using std::thread;
using std::shared_ptr;

namespace rokid {
namespace speech {

extern JavaVM* vm_;
static const char* tag_ = "asr.jni";

enum AsrResultFieldAlias {
	RES_ID,
	RES_TYPE,
	RES_ERROR,
	RES_ASR,

	RES_FIELD_NUM
};

typedef struct {
	jmethodID handle_callback;
	jfieldID result_fields[RES_FIELD_NUM];
	jclass result_class;
} AsrNativeConstants;

static AsrNativeConstants constants_;

class AsrPollThread {
public:
	void start(shared_ptr<Asr> asr, jobject asr_obj) {
		asr_ = asr;
		asr_obj_ = asr_obj;
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
		env_->DeleteGlobalRef(asr_obj_);
		vm_->DetachCurrentThread();
	}

	void do_poll() {
		AsrResult result;
		int32_t r;
		jobject res_obj;
		while (true) {
			if (!asr_->poll(result))
				break;
			env_->PushLocalFrame(32);
			res_obj = generate_result_object(result);
			env_->CallVoidMethod(asr_obj_, constants_.handle_callback, res_obj);
			env_->PopLocalFrame(NULL);
		}
	}

private:
	jobject generate_result_object(AsrResult& result) {
		jobject res_obj = env_->AllocObject(constants_.result_class);
		env_->SetIntField(res_obj, constants_.result_fields[RES_ID], result.id);
		env_->SetIntField(res_obj, constants_.result_fields[RES_TYPE], result.type);
		env_->SetIntField(res_obj, constants_.result_fields[RES_ERROR], result.err);
		jstring strobj;
		if (result.asr.length() > 0) {
			strobj = env_->NewStringUTF(result.asr.c_str());
			env_->SetObjectField(res_obj, constants_.result_fields[RES_ASR], strobj);
		}
		return res_obj;
	}

private:
	thread* thread_;
	shared_ptr<Asr> asr_;
	jobject asr_obj_;
	JNIEnv* env_;
};

typedef struct {
	shared_ptr<Asr> asr;
	AsrPollThread* poll_thread;
} AsrNativeInfo;

static void com_rokid_speech_Asr__sdk_init(JNIEnv *env, jobject thiz, jclass asr_cls, jclass res_cls) {
	assert(vm_);
	constants_.handle_callback = env->GetMethodID(
			asr_cls, "handle_callback", "(Lcom/rokid/speech/Asr$AsrResult;)V");
	constants_.result_fields[RES_ID] = env->GetFieldID(res_cls, "id", "I");
	constants_.result_fields[RES_TYPE] = env->GetFieldID(res_cls, "type", "I");
	constants_.result_fields[RES_ERROR] = env->GetFieldID(res_cls, "err", "I");
	constants_.result_fields[RES_ASR] = env->GetFieldID(res_cls, "asr", "Ljava/lang/String;");
	constants_.result_class = (jclass)env->NewGlobalRef(res_cls);
}

static jlong com_rokid_speech_Asr__sdk_create(JNIEnv *env, jobject thiz) {
	AsrNativeInfo* r = new AsrNativeInfo();
	memset(r, 0, sizeof(*r));
	r->asr = new_asr();
	return (jlong)r;
}

static void com_rokid_speech_Asr__sdk_delete(JNIEnv *env, jobject thiz, jlong asrl) {
	AsrNativeInfo* p = reinterpret_cast<AsrNativeInfo*>(asrl);
	if (p)
		delete p;
}

static jboolean com_rokid_speech_Asr__sdk_prepare(JNIEnv *env, jobject thiz, jlong asrl) {
	AsrNativeInfo* p = reinterpret_cast<AsrNativeInfo*>(asrl);
	assert(p);
	if (!p->asr->prepare()) {
		Log::d(tag_, "prepare failed");
		return false;
	}
	p->poll_thread = new AsrPollThread();
	p->poll_thread->start(p->asr, env->NewGlobalRef(thiz));
	return true;
}

static void com_rokid_speech_Asr__sdk_release(JNIEnv *env, jobject thiz, jlong asrl) {
	AsrNativeInfo* p = reinterpret_cast<AsrNativeInfo*>(asrl);
	assert(p);
	p->asr->release();
	if (p->poll_thread) {
		p->poll_thread->close();
		delete p->poll_thread;
		p->poll_thread = NULL;
	}
}

static jint com_rokid_speech_Asr__sdk_start_voice(JNIEnv *env, jobject thiz, jlong asrl) {
	AsrNativeInfo* p = reinterpret_cast<AsrNativeInfo*>(asrl);
	assert(p);
	return p->asr->start();
}

static void com_rokid_speech_Asr__sdk_put_voice(JNIEnv *env,
		jobject thiz, jlong asrl, jint id, jbyteArray voice, jint offset, jint length) {
	AsrNativeInfo* p = reinterpret_cast<AsrNativeInfo*>(asrl);
	assert(p);
	jbyte* buf = new jbyte[length];
	env->GetByteArrayRegion(voice, offset, length, buf);
	p->asr->voice(id, (uint8_t*)buf, length);
	delete buf;
}

static void com_rokid_speech_Asr__sdk_end_voice(JNIEnv *env,
		jobject thiz, jlong asrl, jint id) {
	AsrNativeInfo* p = reinterpret_cast<AsrNativeInfo*>(asrl);
	assert(p);
	p->asr->end(id);
}

static void com_rokid_speech_Asr__sdk_cancel(JNIEnv *env, jobject thiz, jlong asrl, jint id) {
	AsrNativeInfo* p = reinterpret_cast<AsrNativeInfo*>(asrl);
	assert(p);
	p->asr->cancel(id);
}

static void com_rokid_speech_Asr__sdk_config(JNIEnv *env, jobject thiz, jlong asrl, jstring key, jstring value) {
	AsrNativeInfo* p = reinterpret_cast<AsrNativeInfo*>(asrl);
	assert(p);
	const char* keystr = env->GetStringUTFChars(key, NULL);
	const char* valstr = env->GetStringUTFChars(value, NULL);
	p->asr->config(keystr, valstr);
	env->ReleaseStringUTFChars(key, keystr);
	env->ReleaseStringUTFChars(value, valstr);
}

static JNINativeMethod _nmethods[] = {
	{ "_sdk_init", "(Ljava/lang/Class;Ljava/lang/Class;)V", (void*)com_rokid_speech_Asr__sdk_init },
	{ "_sdk_create", "()J", (void*)com_rokid_speech_Asr__sdk_create },
	{ "_sdk_delete", "(J)V", (void*)com_rokid_speech_Asr__sdk_delete },
	{ "_sdk_prepare", "(J)Z", (void*)com_rokid_speech_Asr__sdk_prepare },
	{ "_sdk_release", "(J)V", (void*)com_rokid_speech_Asr__sdk_release },
	{ "_sdk_start_voice", "(J)I", (void*)com_rokid_speech_Asr__sdk_start_voice },
	{ "_sdk_put_voice", "(JI[BII)V", (void*)com_rokid_speech_Asr__sdk_put_voice },
	{ "_sdk_end_voice", "(JI)V", (void*)com_rokid_speech_Asr__sdk_end_voice },
	{ "_sdk_cancel", "(JI)V", (void*)com_rokid_speech_Asr__sdk_cancel },
	{ "_sdk_config", "(JLjava/lang/String;Ljava/lang/String;)V", (void*)com_rokid_speech_Asr__sdk_config },
};

int register_com_rokid_speech_Asr(JNIEnv* env) {
	const char* kclass = "com/rokid/speech/Asr";
	jclass target = env->FindClass(kclass);
	if (target == NULL) {
		Log::e("find class for %s failed", kclass);
		return -1;
	}
	return jniRegisterNativeMethods(env, kclass, _nmethods, NELEM(_nmethods));
}

} // namespace speech
} // namespace rokid
