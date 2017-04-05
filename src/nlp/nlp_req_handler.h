#pragma once

#include "pipeline_handler.h"
#include "nlp.h"
#include "types.h"
#include "speech.grpc.pb.h"
#include "nlp_cancel_handler.h"

namespace rokid {
namespace speech {

class NlpReqHandler : public PipelineHandler<NlpReqInfo, NlpResult> {
public:
	NlpReqHandler();

	void set_grpc_stub(std::unique_ptr<rokid::open::Speech::Stub>& stub);

	inline void set_cancel_handler(NlpCancelHandler* handler) {
		cancel_handler_ = handler;
	}

	std::shared_ptr<NlpResult> poll();

	bool closed();

protected:
	void start_handle(std::shared_ptr<NlpReqInfo> in, void* arg);

	int32_t handle(std::shared_ptr<NlpReqInfo> in, void* arg);

	void end_handle(std::shared_ptr<NlpReqInfo> in, void* arg);

private:
	std::unique_ptr<rokid::open::Speech::Stub> stub_;
	std::list<std::shared_ptr<NlpResult> > responses_;
	NlpCancelHandler* cancel_handler_;
};

} // namespace speech
} // namespace rokid
