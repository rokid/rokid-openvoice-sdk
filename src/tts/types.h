#pragma once

#include <grpc++/client_context.h>
#include "grpc++/impl/codegen/sync_stream.h"
#include "speech.pb.h"
#include "speech_config.h"

namespace rokid {
namespace speech {

typedef struct {
	int32_t id;
	bool deleted;
	std::shared_ptr<std::string> data;
} TtsReqInfo;

typedef struct {
	int32_t current_id;
	grpc::ClientContext* context;
	SpeechConfig config;
} CommonArgument;

typedef grpc::ClientReader<rokid::open::TtsResponse> TtsRespStream;

#define tag__ "speech.tts"

} // namespace speech
} // namespace rokid
