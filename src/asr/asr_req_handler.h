#pragma once

#include <memory>
#include <list>
#include <mutex>
#include <condition_variable>
#include "pipeline_handler.h"
#include "types.h"
#include "speech.grpc.pb.h"
#include "asr_cancel_handler.h"

namespace rokid {
namespace speech {

class AsrReqHandler : public PipelineHandler<AsrReqInfo, AsrClientStream> {
public:
	AsrReqHandler();

	AsrClientStreamSp poll();

	void close();

	inline void set_grpc_stub(std::unique_ptr<rokid::open::Speech::Stub>& stub) {
		stub_ = std::move(stub);
	}

	inline void set_cancel_handler(AsrCancelHandler* handler) {
		cancel_handler_ = handler;
	}

	bool closed();

protected:
	void start_handle(std::shared_ptr<AsrReqInfo> in, void* arg);

	int32_t handle(std::shared_ptr<AsrReqInfo> in, void* arg);

	void end_handle(std::shared_ptr<AsrReqInfo> in, void* arg);

private:
	std::unique_ptr<rokid::open::Speech::Stub> stub_;
	std::list<AsrClientStreamSp> resp_streams_;
	std::mutex mutex_;
	std::condition_variable cond_;
	bool closed_;
	AsrCancelHandler* cancel_handler_;
};

} // namespace speech
} // namespace rokid
