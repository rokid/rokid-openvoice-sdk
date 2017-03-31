#include <grpc++/channel.h>
#include <grpc++/create_channel.h>
#include "tts_runner.h"
#include "speech_connection.h"
#include "log.h"

using grpc::Status;
using rokid::open::TtsHeader;
using rokid::open::TtsRequest;

namespace rokid {
namespace speech {

const char* TtsRunner::tag_ = "speech.TtsRunner";

TtsRunner::TtsRunner() : context_(NULL), config_(NULL) {
}

TtsRunner::~TtsRunner() {
	if (context_)
		delete context_;
}

bool TtsRunner::prepare(SpeechConfig* config) {
	if (!TtsRunnerBase::prepare()) {
		Log::d(tag_, "TtsRunnerBase prepare failed");
		return false;
	}
	stub_ = SpeechConnection::connect(config, "tts");
	if (stub_.get() == NULL) {
		Log::d(tag_, "SpeechConnection for tts failed");
		TtsRunnerBase::close();
		return false;
	}
	config_ = config;
	return true;
}

void TtsRunner::close() {
	TtsRunnerBase::close();
}

ClientReaderSp TtsRunner::do_request(int id, shared_ptr<string> data) {
	TtsRequest req;
	TtsHeader* header;

	context_ = new ClientContext();
	header = new TtsHeader();
	header->set_id(id);
	const char* codec = config_->get("codec", "pcm");
	header->set_codec(codec);
	header->set_declaimer("zh");
	req.set_allocated_header(header);
	req.set_text(*data);
	Log::d(tag_, "do request %d: %s", id, data->c_str());
	return stub_->tts(context_, req);
}

void TtsRunner::do_stream_response(int id, ClientReaderSp stream,
		PendingStreamQueue<TtsResponse>* responses) {
	shared_ptr<TtsResponse> res;
	responses->start(id);
	while (true) {
		res.reset(new TtsResponse());
		if (!stream->Read(res.get()))
			break;
		Log::d(tag_, "read tts voice: %d", res->voice().size());
		responses->stream(id, res);
	}
	Status status = stream->Finish();
	if (context_) {
		delete context_;
		context_ = NULL;
	}
	if (!status.ok()) {
		Log::d(tag_, "grpc error: %d, %s", status.error_code(), status.error_message().c_str());
		uint32_t err = 1;
		responses->erase(id, err);
	} else
		responses->end(id);
}

} // namespace speech
} // namespace rokid
