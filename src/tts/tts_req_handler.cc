#include <chrono>
#include "speech_connection.h"
#include "log.h"
#include "tts_req_handler.h"
#include "tts_impl.h"

#define WS_SEND_TIMEOUT 5

using std::shared_ptr;
using rokid::open::TtsRequest;

namespace rokid {
namespace speech {

TtsReqHandler::TtsReqHandler() : cancel_handler_(NULL) {
}

shared_ptr<TtsRespInfo> TtsReqHandler::poll() {
	return current_resp_;
}

void TtsReqHandler::start_handle(shared_ptr<TtsReqInfo> in, void* arg) {
}

int32_t TtsReqHandler::handle(shared_ptr<TtsReqInfo> in, void* arg) {
	if (!in.get())
		return FLAG_ERROR;
	TtsCommonArgument* carg = (TtsCommonArgument*)arg;
	if (in->deleted) {
		cancel_handler_->cancelled(in->id);
		return FLAG_BREAK_LOOP;
	}

	TtsRequest req;
	req.set_id(in->id);
	const char* v = carg->config.get("codec", "pcm");
	req.set_codec(v);
	v = carg->config.get("declaimer", "zh");
	req.set_declaimer(v);
	req.set_text(*in->data);
	shared_ptr<SpeechConnection> conn = carg->keepalive_.get_conn(0);
	Log::i(tag__, "TtsReqHandler: get speech conneciton %p", conn.get());
	TtsError err = TtsError::TTS_SUCCESS;
	if (conn.get() == NULL) {
		err = TtsError::TTS_SDK_CLOSED;
		goto exit;
	}
	if (!conn->send(req, WS_SEND_TIMEOUT)) {
		Log::i(tag__, "TtsReqHandler: send request failed, shutdown"
				"connection, try reconnect");
		carg->keepalive_.shutdown(conn.get());
		err = TtsError::TTS_SERVICE_UNAVAILABLE;
		goto exit;
	}

exit:
	current_resp_.reset(new TtsRespInfo());
	current_resp_->id = in->id;
	current_resp_->err = err;
	return 0;
}

void TtsReqHandler::end_handle(shared_ptr<TtsReqInfo> in, void* arg) {
	// do nothing
}

bool TtsReqHandler::closed() {
	return false;
}

} // namespace speech
} // namespace rokid
