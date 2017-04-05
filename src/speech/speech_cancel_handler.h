#pragma once

#include <list>
#include <memory>
#include "speech.h"
#include "cancel_pipeline_handler.h"

namespace rokid {
namespace speech {

class SpeechCancelHandler : public CancelPipelineHandler<SpeechResult, SpeechResult> {
public:
	int32_t handle_not_cancel(std::shared_ptr<SpeechResult> in, void* arg);

	std::shared_ptr<SpeechResult> poll_not_block();

	std::shared_ptr<SpeechResult> generate_cancel_result(int32_t id);

private:
	std::list<std::shared_ptr<SpeechResult> > responses_;
};

} // namespace speech
} // namespace rokid
