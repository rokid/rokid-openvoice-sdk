#pragma once

#include <grpc/grpc.h>
#include <grpc++/client_context.h>
#include "grpc_runner.h"
#include "speech.pb.h"
#include "speech.grpc.pb.h"
#include "speech_config.h"

using namespace std;
using grpc::ClientContext;
using grpc::ClientReader;
using rokid::open::TtsResponse;
using rokid::open::Speech;

namespace rokid {
namespace speech {

typedef GrpcRunner<string, ClientReader<TtsResponse>, TtsResponse> TtsRunnerBase;
typedef shared_ptr<ClientReader<TtsResponse> > ClientReaderSp;

class TtsRunner : public TtsRunnerBase {
public:
	TtsRunner();

	~TtsRunner();

	bool prepare(SpeechConfig* config);

	void close();

protected:
	ClientReaderSp do_request(int id, shared_ptr<string> data);

	bool request_finish(int id, ClientReaderSp stream);

	void do_stream_response(int id, ClientReaderSp stream,
			PendingStreamQueue<TtsResponse>* responses);

private:
	static const char* tag_;

	ClientContext* context_;
	unique_ptr<Speech::Stub> stub_;
	SpeechConfig* config_;
};

} // namespace speech
} // namespace rokid
