#include <jni.h>
#include "JNIHelp.h"
#include "log.h"

namespace rokid {
namespace speech {

JavaVM* vm_ = NULL;
int register_com_rokid_speech_Tts(JNIEnv* env);
int register_com_rokid_speech_Asr(JNIEnv* env);
int register_com_rokid_speech_Speech(JNIEnv* env);
int register_com_rokid_speech_SpeechOptions(JNIEnv* env);

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
	rokid::speech::register_com_rokid_speech_Asr(env);
	rokid::speech::register_com_rokid_speech_Speech(env);
	rokid::speech::register_com_rokid_speech_SpeechOptions(env);
	return JNI_VERSION_1_4;
}
