#pragma once

#include "callback.h"
#include "asr_runner.h"
#include "speech_config.h"

using std::string;

namespace rokid {
namespace speech {

class AsrCallback {
public:
	virtual ~AsrCallback() { }

	virtual void onStart(int id) = 0;

	virtual void onData(int id, const char* text) = 0;

	virtual void onStop(int id) = 0;

	virtual void onComplete(int id) = 0;

	virtual void onError(int id, int err) = 0;
};

class AsrCallbackManager : public CallbackManagerStream<AsrResponse, AsrCallback> {
public:
	static const char* tag_;

protected:
	void onData(AsrCallback* cb, int id, const AsrResponse* data);
};

class Asr {
public:
	Asr();

	~Asr();

	bool prepare();

	bool prepared();

	void release();

	int start(AsrCallback* cb);

	void voice(int id, const unsigned char* data, int length);

	void end(int id);

	void cancel(int id);

	void config(const char* key, const char* value);

private:
	AsrRunner runner_;
	AsrCallbackManager callback_manager_;
	PendingStreamQueue<string>* requests_;
	SpeechConfig* config_;
};

} // namespace speech
} // namespace rokid
