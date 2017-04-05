#pragma once

#include <mutex>
#include <condition_variable>
#include "pipeline_handler.h"
#include "types.h"
#include "pending_queue.h"
#include "speech_cancel_handler.h"

namespace rokid {
namespace speech {

class SpeechReqProvider : public PipelineOutHandler<SpeechReqInfo> {
public:
	SpeechReqProvider();

	std::shared_ptr<SpeechReqInfo> poll();

	void close();

	bool closed();

	void put_text(int32_t id, const char* text);

	void start_voice(int32_t id);

	void put_voice(int32_t id, const uint8_t* data, uint32_t length);

	void end_voice(int32_t id);

	void cancel(int32_t id);

	inline void set_cancel_handler(SpeechCancelHandler* handler) {
		cancel_handler_ = handler;
	}

private:
	std::list<std::shared_ptr<SpeechReqInfo> > text_reqs_;
	StreamQueue<std::string> voice_reqs_;
	std::mutex mutex_;
	std::condition_variable cond_;
	bool closed_;
	SpeechCancelHandler* cancel_handler_;
};

} // namespace speech
} // namespace rokid
