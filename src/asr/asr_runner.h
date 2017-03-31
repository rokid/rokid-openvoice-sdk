#pragma once

#include <grpc/grpc.h>
#include <grpc++/client_context.h>
#include "grpc_runner.h"
#include "speech.pb.h"
#include "speech.grpc.pb.h"
#include "speech_config.h"

using std::string;
using std::shared_ptr;
using grpc::ClientContext;
using grpc::ClientReaderWriter;
using rokid::open::AsrRequest;
using rokid::open::AsrResponse;
using rokid::open::Speech;

namespace rokid {
namespace speech {

typedef ClientReaderWriter<AsrRequest, AsrResponse> AsrClientStream;
typedef GrpcRunnerStream<string, AsrClientStream, AsrResponse> AsrRunnerBase;
typedef shared_ptr<AsrClientStream> AsrClientStreamSp;
typedef PendingStreamQueue<AsrResponse> ResponseQueue;

class AsrRunner : public AsrRunnerBase {
public:
	AsrRunner();

	bool prepare(SpeechConfig* config);

	void close();

protected:
	AsrClientStreamSp do_stream_start(int id);

	void do_stream_data(int id, AsrClientStreamSp stream, shared_ptr<string> data);

	void do_stream_end(int id, AsrClientStreamSp stream);

	void do_stream_response(int id, AsrClientStreamSp stream,
			ResponseQueue* responses);

private:
	ClientContext* context_;
	unique_ptr<Speech::Stub> stub_;
	SpeechConfig* config_;
};

} // namespace speech
} // namespace rokid
