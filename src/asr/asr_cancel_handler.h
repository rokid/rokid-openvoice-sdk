#pragma once

#include "asr.h"
#include "cancel_pipeline_handler.h"

namespace rokid {
namespace speech {

class AsrCancelHandler : public CancelPipelineHandler<AsrResult, AsrResult> {
public:
	int32_t handle_not_cancel(std::shared_ptr<AsrResult> in, void* arg);

	std::shared_ptr<AsrResult> poll_not_block();

	std::shared_ptr<AsrResult> generate_cancel_result(int32_t id);

private:
	std::list<std::shared_ptr<AsrResult> > responses_;
};

} // namespace speech
} // namespace rokid
