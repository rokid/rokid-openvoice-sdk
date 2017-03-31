#pragma once

#include "callback.h"
#include "nlp_runner.h"
#include "speech.pb.h"
#include "speech_config.h"

namespace rokid {
namespace speech {

class NlpCallback {
public:
	virtual ~NlpCallback() { }

	virtual void onNlp(int id, const char* nlp) = 0;

	virtual void onStop(int id) = 0;

	virtual void onError(int id, int err) = 0;
};

class NlpCallbackManager : public CallbackManagerUnary<NlpRespItem, NlpCallback> {
protected:
	void onData(NlpCallback* cb, int id, const NlpRespItem* data);
};

class Nlp {
public:
	Nlp();

	~Nlp();

	bool prepare();

	bool prepared();

	void release();

	int request(const char* asr, NlpCallback* cb);

	void cancel(int32_t id);

	void config(const char* key, const char* value);

private:
	NlpRunner runner_;
	NlpCallbackManager callback_manager_;
	PendingQueue<string>* requests_;
	SpeechConfig* config_;
};

} // namespace speech
} // namespace rokid
