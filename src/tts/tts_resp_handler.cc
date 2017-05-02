#include "tts_resp_handler.h"
#include "log.h"

#define WS_RECV_TIMEOUT 10

using std::shared_ptr;
using rokid::open::TtsResponse;
using rokid::open::SpeechErrorCode;

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
	res->err = (TtsError)err;
	res->id = id;
	res->voice = voice;
	return res;
}

bool TtsRespHandler::closed() {
	return false;
}

void TtsRespHandler::start_handle(shared_ptr<TtsRespInfo> in, void* arg) {
	start_stream_ = true;
}

int32_t TtsRespHandler::handle(shared_ptr<TtsRespInfo> in, void* arg) {
	TtsResponse resp;
	TtsCommonArgument* carg = (TtsCommonArgument*)arg;

	if (start_stream_) {
		start_stream_ = false;
		responses_.start(in->id);
		Log::d(tag__, "TtsRespHandler: %d start voice",
				in->id);
		return FLAG_NOT_POLL_NEXT | FLAG_AS_HEAD;
	}

	if (in->err != TtsError::TTS_SUCCESS) {
		responses_.erase(in->id, in->err);
		return 0;
	}

	shared_ptr<SpeechConnection> conn = carg->keepalive_.get_conn(0);
	Log::d(tag__, "TtsRespHandler: get speech connection %p", conn.get());
	if (conn.get() == NULL) {
		responses_.erase(in->id, TtsError::TTS_SDK_CLOSED);
		return 0;
	}
	if (!conn->recv(resp, WS_RECV_TIMEOUT)) {
		Log::d(tag__, "TtsRespHandler: recv tts data failed, shutdown "
				"connection and try reconnect");
		carg->keepalive_.shutdown(conn.get());
		responses_.erase(in->id, TtsError::TTS_SERVICE_UNAVAILABLE);
		return 0;
	}
	SpeechErrorCode result = resp.result();
	if (result == SpeechErrorCode::SUCCESS) {
		int32_t r = FLAG_NOT_POLL_NEXT | FLAG_AS_HEAD | FLAG_BREAK_LOOP;
		if (resp.has_voice()) {
#ifdef LOW_PB_VERSION
			shared_ptr<string> voice(new string(resp.voice()));
#else
			shared_ptr<string> voice(resp.release_voice());
#endif
			responses_.stream(in->id, voice);
			Log::d(tag__, "TtsRespHandler: %d read voice %d bytes",
					in->id, voice->length());
			r = FLAG_NOT_POLL_NEXT | FLAG_AS_HEAD;
		}
		if (resp.has_finish() && resp.finish()) {
			responses_.end(in->id);
			Log::d(tag__, "TtsRespHandler: %d read voice end", in->id);
			r = 0;
		}
		return r;
	}
	responses_.erase(in->id, result);
	return 0;
}

void TtsRespHandler::end_handle(shared_ptr<TtsRespInfo> in, void* arg) {
}

} // namespace speech
} // namespace rokid
