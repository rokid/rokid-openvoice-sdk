#pragma once

#include <stdint.h>
#include <string>
#include <map>
#include "tts_runner.h"
#include "callback.h"
#include "speech_config.h"

namespace rokid {
namespace speech {

using std::string;
using std::map;

class TtsCallback;

class TtsCallbackManager : public CallbackManagerStream<TtsResponse, TtsCallback> {
protected:
	void onData(TtsCallback* cb, int32_t id, const TtsResponse* data);
};

class Tts {
public:
	Tts();

	~Tts();

	bool prepare(const char* config_file = NULL);

	void release();

	bool prepared();

	int32_t speak(const char* content, TtsCallback* cb);

	void stop(int32_t id = 0);

	// 'key': declaimer        'value':
	//        codec            'value': opu2  (default)
	//                                  opu
	//                                  pcm
	void config(const char* key, const char* value);

private:
	static void* callback_routine(void* arg);

	bool config_valid(const char* key, const char* value);

	void callback_loop();

public:
	static const char* tag_;

private:
	SpeechConfig* config_;
	PendingQueue<string>* requests_;
	TtsRunner runner_;
	TtsCallbackManager callback_manager_;
};

class TtsCallback {
public:
	virtual ~TtsCallback() { }

	virtual void onStart(int32_t id) = 0;

	virtual void onText(int32_t id, const char* text) = 0;

	virtual void onVoice(int32_t id, const uint8_t* data, int length) = 0;

	virtual void onStop(int32_t id) = 0;

	virtual void onComplete(int32_t id) = 0;

	virtual void onError(int32_t id, int32_t err) = 0;
};

// debug
// void printHexData(uint8_t* data, uint32_t len);

} // namespace speech
} // namespace rokid
