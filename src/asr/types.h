#pragma once

#include <memory>
#include "grpc++/impl/codegen/sync_stream.h"
#include "grpc++/impl/codegen/client_context.h"
#include "speech.pb.h"
#include "speech.grpc.pb.h"
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

class AsrCommonArgument {
public:
	SpeechConfig config;
	AsrClientStreamSp stream;

	// see implementation in speech_impl.cc
	std::shared_ptr<rokid::open::Speech::Stub> stub();

	void reset_stub();

private:
	std::shared_ptr<rokid::open::Speech::Stub> stub_;
	std::mutex mutex_;
};

#define tag__ "speech.asr"

} // namespace speech
} // namespace rokid
