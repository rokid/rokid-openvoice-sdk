#pragma once

#include "pending_queue.h"
#include "pipeline_handler.h"
#include "tts.h"
#include "types.h"

namespace rokid {
namespace speech {

class TtsRespHandler : public PipelineHandler<TtsRespInfo, TtsResult> {
public:
	TtsRespHandler();

	std::shared_ptr<TtsResult> poll();

	bool closed();

protected:
	void start_handle(std::shared_ptr<TtsRespInfo> in, void* arg);

	int32_t handle(std::shared_ptr<TtsRespInfo> in, void* arg);

	void end_handle(std::shared_ptr<TtsRespInfo> in, void* arg);

private:
	StreamQueue<std::string> responses_;
	bool start_stream_;
};

} // namespace speech
} // namespace rokid
