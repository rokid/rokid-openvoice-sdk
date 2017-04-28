#pragma once

#include <grpc++/client_context.h>
#include "grpc++/impl/codegen/sync_stream.h"
#include "speech.pb.h"
#include "speech.grpc.pb.h"
#include "speech_config.h"

namespace rokid {
namespace speech {

typedef struct {
	int32_t id;
	bool deleted;
	std::shared_ptr<std::string> data;
} TtsReqInfo;

class CommonArgument {
public:
	int32_t current_id;
	grpc::ClientContext* context;
	SpeechConfig config;

	// see implementation in tts_impl.cc
	std::shared_ptr<rokid::open::Speech::Stub> stub();

	inline void reset_stub() {
		stub_.reset();
	}

private:
	std::shared_ptr<rokid::open::Speech::Stub> stub_;
};

typedef grpc::ClientReader<rokid::open::TtsResponse> TtsRespStream;

#define tag__ "speech.tts"

} // namespace speech
} // namespace rokid
