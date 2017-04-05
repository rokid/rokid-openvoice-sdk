#pragma once

#include <string>
#include <memory>
#include "pipeline_handler.h"
#include "speech.pb.h"
#include "pending_queue.h"
#include "types.h"

namespace rokid {
namespace speech {

class AsrReqProvider : public PipelineOutHandler<AsrReqInfo> {
public:
	// override from PipelineOutHandler
	std::shared_ptr<AsrReqInfo> poll();

	inline PendingStreamQueue<std::string>* queue() {
		return &requests_;
	}

	bool closed();

private:
	PendingStreamQueue<std::string> requests_;
};

} // namespace speech
} // namespace rokid
