#pragma once

#include <memory>
#include "grpc++/impl/codegen/sync_stream.h"
#include "grpc++/impl/codegen/client_context.h"
#include "speech.pb.h"
#include "speech_config.h"

typedef grpc::ClientReaderWriter<rokid::open::AsrRequest, rokid::open::AsrResponse> AsrClientStream;
typedef std::shared_ptr<AsrClientStream> AsrClientStreamSp;

namespace rokid {
namespace speech {

class AsrReqInfo {
public:
	int32_t id;
	uint32_t type;
	std::shared_ptr<std::string> voice;
};

class AsrRespInfo {
public:
	int32_t id;
	AsrClientStreamSp stream;
	std::shared_ptr<grpc::ClientContext> context;
};

typedef struct {
	SpeechConfig config;
	AsrClientStreamSp stream;
} CommonArgument;

#define tag__ "speech.asr"

} // namespace speech
} // namespace rokid
