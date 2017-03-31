#pragma once

#include "grpc_runner.h"
#include "speech.grpc.pb.h"
#include "speech_config.h"

using std::string;
using rokid::open::Speech;

namespace rokid {
namespace speech {

class NlpRespItem {
public:
	// 0: nlp string
	// 1: stop
	// 2: error
	uint32_t type;
	uint32_t error;
	string nlp;
};

typedef GrpcRunnerSync<string, NlpRespItem> NlpRunnerBase;

class NlpRunner : public NlpRunnerBase {
public:
	bool prepare(SpeechConfig* config);

	void close();

protected:
	void do_request(int32_t id, shared_ptr<string> data);

private:
	unique_ptr<Speech::Stub> stub_;
};

} // namespace speech
} // namespace rokid
