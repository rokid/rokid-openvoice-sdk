#pragma once

#include <memory>
#include "pipeline_handler.h"
#include "speech_config.h"
#include "speech.grpc.pb.h"
#include "types.h"
#include "tts_cancel_handler.h"

namespace rokid {
namespace speech {

class TtsReqHandler : public PipelineHandler<TtsReqInfo, TtsRespStream> {
public:
	TtsReqHandler();

	bool prepare(SpeechConfig* config);

	std::shared_ptr<TtsRespStream> poll();

	void close();

	bool closed();

	inline void set_cancel_handler(TtsCancelHandler* handler) {
		cancel_handler_ = handler;
	}

protected:
	void start_handle(std::shared_ptr<TtsReqInfo> in, void* arg);

	int32_t handle(std::shared_ptr<TtsReqInfo> in, void* arg);

	void end_handle(std::shared_ptr<TtsReqInfo> in, void* arg);

private:
	std::unique_ptr<rokid::open::Speech::Stub> stub_;
	std::shared_ptr<TtsRespStream> stream_;
	TtsCancelHandler* cancel_handler_;
};

} // namespace speech
} // namespace rokid
