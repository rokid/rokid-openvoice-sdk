#pragma once

#include "pipeline_handler.h"
#include "pending_queue.h"
#include "types.h"

namespace rokid {
namespace speech {

class NlpReqProvider : public PipelineOutHandler<NlpReqInfo> {
public:
	std::shared_ptr<NlpReqInfo> poll();

	bool closed();

	inline PendingQueue<std::string>* queue() {
		return &requests_;
	}

private:
	PendingQueue<std::string> requests_;
};

} // namespace speech
} // namespace rokid
