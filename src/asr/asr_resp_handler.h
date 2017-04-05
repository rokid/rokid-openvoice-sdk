#pragma once

#include "pipeline_handler.h"
#include "pending_queue.h"
#include "types.h"
#include "asr.h"

namespace rokid {
namespace speech {

class AsrRespHandler : public PipelineHandler<AsrClientStream, AsrResult> {
public:
	AsrRespHandler();

	std::shared_ptr<AsrResult> poll();

	bool closed();

protected:
	void start_handle(AsrClientStreamSp in, void* arg);

	int32_t handle(AsrClientStreamSp in, void* arg);

	void end_handle(AsrClientStreamSp in, void* arg);

private:
	StreamQueue<std::string> responses_;
	bool start_stream_;
};

} // namespace speech
} // namespace rokid
