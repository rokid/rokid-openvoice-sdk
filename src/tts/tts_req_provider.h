#pragma once

#include <string>
#include <memory>
#include "pipeline_handler.h"
#include "pending_queue.h"
#include "types.h"

namespace rokid {
namespace speech {

class TtsReqProvider : public PipelineOutHandler<TtsReqInfo> {
public:
	// override from PipelineOutHandler
	std::shared_ptr<TtsReqInfo> poll();

	inline PendingQueue<std::string>* queue() {
		return &requests_;
	}

	bool closed();

private:
	PendingQueue<std::string> requests_;
};

} // namespace speech
} // namespace rokid
