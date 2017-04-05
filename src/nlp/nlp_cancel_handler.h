#pragma once

#include "cancel_pipeline_handler.h"
#include "nlp.h"

namespace rokid {
namespace speech {

class NlpCancelHandler : public CancelPipelineHandler<NlpResult, NlpResult> {
public:
	int32_t handle_not_cancel(std::shared_ptr<NlpResult> in, void* arg);

	std::shared_ptr<NlpResult> poll_not_block();

	std::shared_ptr<NlpResult> generate_cancel_result(int32_t id);

private:
	std::list<std::shared_ptr<NlpResult> > responses_;
};

} // namespace speech
} // namespace rokid
