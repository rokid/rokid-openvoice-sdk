#include "asr_runner.h"
#include "speech_connection.h"

using grpc::Status;
using rokid::open::AsrHeader;

namespace rokid {
namespace speech {

AsrRunner::AsrRunner() : context_(NULL), config_(NULL) {
}

bool AsrRunner::prepare(SpeechConfig* config) {
	if (!AsrRunnerBase::prepare())
		return false;
	stub_ = SpeechConnection::connect(config, "asr");
	if (stub_.get() == NULL) {
		AsrRunnerBase::close();
		return false;
	}
	config_ = config;
	return true;
}

void AsrRunner::close() {
	AsrRunnerBase::close();
}

AsrClientStreamSp AsrRunner::do_stream_start(int id) {
	context_ = new ClientContext();
	AsrClientStreamSp stream(stub_->asr(context_));
	if (stream.get() == NULL)
		return NULL;
	AsrRequest req;
	AsrHeader* header = req.mutable_header();
	header->set_id(id);
	header->set_lang("zh");
	const char* codec = config_->get("codec", "pcm");
	header->set_codec(codec);
	stream->Write(req);
	return stream;
}

void AsrRunner::do_stream_data(int id, AsrClientStreamSp stream, shared_ptr<string> data) {
	AsrRequest req;
	req.set_voice(data->data(), data->length());
	stream->Write(req);
}

void AsrRunner::do_stream_end(int id, AsrClientStreamSp stream) {
	stream->WritesDone();
}

void AsrRunner::do_stream_response(int id, AsrClientStreamSp stream,
		ResponseQueue* responses) {
	shared_ptr<AsrResponse> res;
	responses->start(id);
	while (true) {
		res.reset(new AsrResponse());
		if (!stream->Read(res.get()))
			break;
		Log::d(tag_, "read asr result: %d", res->asr().length());
		responses->stream(id, res);
	}
	Status status = stream->Finish();
	if (context_) {
		delete context_;
		context_ = NULL;
	}
	if (status.ok())
		responses->end(id);
	else {
		Log::d(tag_, "asr rpc error: %d, %s", status.error_code(), status.error_message().c_str());
		responses->erase(id, 1);
	}
}

} // namespace speech
} // namespace rokid
