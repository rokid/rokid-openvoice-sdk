#pragma once

#include "speech.h"
#include "pipeline.h"
#include "types.h"
#include "speech_req_provider.h"
#include "speech_req_handler.h"
#include "speech_resp_handler.h"
#include "speech_cancel_handler.h"

namespace rokid {
namespace speech {

class SpeechImpl : public Speech, public Pipeline<SpeechReqInfo> {
public:
	SpeechImpl();

	bool prepare();

	void release();

	int32_t put_text(const char* text);

	int32_t start_voice();

	void put_voice(int32_t id, const uint8_t* data, uint32_t length);

	void end_voice(int32_t id);

	void cancel(int32_t id);

	bool poll(SpeechResult& res);

	void config(const char* key, const char* value);

private:
	inline int32_t next_id() { return ++next_id_; }

private:
	SpeechReqProvider req_provider_;
	SpeechReqHandler req_handler_;
	SpeechRespHandler resp_handler_;
	SpeechCancelHandler cancel_handler_;
	SpeechCommonArgument pipeline_arg_;
	int32_t next_id_;
};

} // namespace speech
} // namespace rokid
