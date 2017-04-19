#pragma once

#include "pipeline_handler.h"
#include "pending_queue.h"
#include "types.h"
#include "asr.h"

namespace rokid {
namespace speech {

class AsrRespHandler : public PipelineHandler<AsrRespInfo, AsrResult> {
public:
	AsrRespHandler();

	std::shared_ptr<AsrResult> poll();

	bool closed();

protected:
	void start_handle(std::shared_ptr<AsrRespInfo> in, void* arg);

	int32_t handle(std::shared_ptr<AsrRespInfo> in, void* arg);

	void end_handle(std::shared_ptr<AsrRespInfo> in, void* arg);

private:
	StreamQueue<std::string> responses_;
	bool start_stream_;
};

} // namespace speech
} // namespace rokid
