#include <grpc++/grpc++.h>
#include <grpc++/impl/codegen/proto_utils.h>
#include "tts_resp_handler.h"
#include "log.h"

using std::shared_ptr;
using grpc::Status;
using rokid::open::TtsResponse;

namespace rokid {
namespace speech {

TtsRespHandler::TtsRespHandler() : start_stream_(false) {
}

shared_ptr<TtsResult> TtsRespHandler::poll() {
	uint32_t err;
	int32_t id;
	shared_ptr<string> voice;
	int32_t r;

	r = responses_.pop(id, voice, err);
	if (r < 0)
		return NULL;
	Log::d(tag__, "TtsRespHandler: poll id %d, type %d",
			id, r);
	shared_ptr<TtsResult> res(new TtsResult());
	res->type = r;
	res->err = err;
	res->id = id;
	res->voice = voice;
	return res;
}

bool TtsRespHandler::closed() {
	return false;
}

void TtsRespHandler::start_handle(shared_ptr<TtsRespStream> in, void* arg) {
	start_stream_ = true;
}

int32_t TtsRespHandler::handle(shared_ptr<TtsRespStream> in, void* arg) {
	TtsResponse resp;
	TtsCommonArgument* carg = (TtsCommonArgument*)arg;

	if (start_stream_) {
		start_stream_ = false;
		responses_.start(carg->current_id);
		Log::d(tag__, "TtsRespHandler: %d start voice",
				carg->current_id);
		return FLAG_NOT_POLL_NEXT | FLAG_AS_HEAD;
	}
	if (in->Read(&resp)) {
		string* tmp = resp.release_voice();
		if (tmp) {
			shared_ptr<string> voice(tmp);
			responses_.stream(carg->current_id, voice);
			Log::d(tag__, "TtsRespHandler: %d read voice %d bytes",
					carg->current_id, voice->length());
			return FLAG_NOT_POLL_NEXT | FLAG_AS_HEAD;
		}
		return FLAG_NOT_POLL_NEXT | FLAG_AS_HEAD | FLAG_BREAK_LOOP;
	}

	Status status = in->Finish();
	if (status.ok()) {
		responses_.end(carg->current_id);
		Log::d(tag__, "TtsRespHandler: %d read voice end",
				carg->current_id);
		return 0;
	}
	Log::d(tag__, "TtsRespHandler: grpc status %d:%s",
			status.error_code(), status.error_message().c_str());
	uint32_t err;
	switch (status.error_code()) {
		case grpc::StatusCode::UNAVAILABLE:
			err = 1;
			// service connection losed, clear the stub,
			// try reconnect in next request
			carg->reset_stub();
			break;
		default:
			// undefined error
			err = 2;
			break;
	}
	responses_.erase(carg->current_id, err);
	return 0;
}

void TtsRespHandler::end_handle(shared_ptr<TtsRespStream> in, void* arg) {
	TtsCommonArgument* carg = (TtsCommonArgument*)arg;
	delete carg->context;
}

} // namespace speech
} // namespace rokid
