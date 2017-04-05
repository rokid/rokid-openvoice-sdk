#pragma once

#include <mutex>
#include <condition_variable>
#include "pipeline_handler.h"
#include "types.h"
#include "speech.grpc.pb.h"
#include "speech_cancel_handler.h"

namespace rokid {
namespace speech {

class SpeechReqHandler : public PipelineHandler<SpeechReqInfo, SpeechRespInfo> {
public:
	SpeechReqHandler();

	void start_handle(std::shared_ptr<SpeechReqInfo> in, void* arg);

	int32_t handle(std::shared_ptr<SpeechReqInfo> in, void* arg);

	void end_handle(std::shared_ptr<SpeechReqInfo> in, void* arg);

	std::shared_ptr<SpeechRespInfo> poll();

	void close();

	bool closed();

	inline void set_grpc_stub(std::unique_ptr<rokid::open::Speech::Stub>& stub) {
		stub_ = std::move(stub);
	}

	inline void set_cancel_handler(SpeechCancelHandler* handler) {
		cancel_handler_ = handler;
	}

private:
	std::list<std::shared_ptr<SpeechRespInfo> > responses_;
	std::mutex mutex_;
	std::condition_variable cond_;
	bool closed_;
	std::unique_ptr<rokid::open::Speech::Stub> stub_;
	SpeechCancelHandler* cancel_handler_;
};

} // namespace speech
} // namespace rokid
