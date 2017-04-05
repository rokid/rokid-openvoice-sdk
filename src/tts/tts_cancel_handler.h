#pragma once

#include <list>
#include "cancel_pipeline_handler.h"
#include "tts.h"

namespace rokid {
namespace speech {

class TtsCancelHandler : public CancelPipelineHandler<TtsResult, TtsResult> {
public:
protected:
	int32_t handle_not_cancel(std::shared_ptr<TtsResult> in, void* arg);

	std::shared_ptr<TtsResult> poll_not_block();

	std::shared_ptr<TtsResult> generate_cancel_result(int32_t id);

private:
	std::list<std::shared_ptr<TtsResult> > responses_;
};

} // namespace speech
} // namespace rokid
