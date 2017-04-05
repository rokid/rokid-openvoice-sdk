#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "types.h"
#include "pipeline_handler.h"

namespace rokid {
namespace speech {

class SpeechRespHandler : public PipelineHandler<SpeechRespInfo, SpeechResult> {
public:
	SpeechRespHandler();

	std::shared_ptr<SpeechResult> poll();

	void close();

	bool closed();

protected:
	void start_handle(std::shared_ptr<SpeechRespInfo> in, void* arg);

	int32_t handle(std::shared_ptr<SpeechRespInfo> in, void* arg);

	void end_handle(std::shared_ptr<SpeechRespInfo> in, void* arg);

private:
	std::list<std::shared_ptr<SpeechResult> > responses_;
	bool start_response_;
	std::mutex mutex_;
	std::condition_variable cond_;
	bool closed_;
};

} // namespace speech
} // namespace rokid
