#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include "grpc++/impl/codegen/sync_stream.h"
#include "grpc++/impl/codegen/client_context.h"
#include "speech.pb.h"
#include "speech.grpc.pb.h"
#include "speech_config.h"
#include "speech.h"

namespace rokid {
namespace speech {

typedef struct {
	int32_t id;
	// 0  text req
	// 1  voice req start
	// 2  voice req end
	// 3  voice req cancel
	// 4  voice req data
	uint32_t type;
	std::shared_ptr<std::string> data;
} SpeechReqInfo;

typedef grpc::ClientReaderWriter<rokid::open::VoiceSpeechRequest, rokid::open::SpeechResponse> SpeechClientStream;
typedef std::shared_ptr<SpeechClientStream> SpeechClientStreamSp;

typedef struct {
	// 0  result of text request
	// 1  grpc client stream
	uint32_t type;
	int32_t id;
	std::shared_ptr<grpc::ClientContext> context;
	SpeechClientStreamSp stream;
	SpeechResult result;
} SpeechRespInfo;

class SpeechCommonArgument {
public:
	SpeechClientStreamSp stream;
	SpeechConfig config;

	// see implementation in speech_impl.cc
	std::shared_ptr<rokid::open::Speech::Stub> stub();

	void reset_stub();

private:
	std::shared_ptr<rokid::open::Speech::Stub> stub_;
	std::mutex mutex_;
};

#define tag__ "speech.speech"

} // namespace speech
} // namespace rokid
