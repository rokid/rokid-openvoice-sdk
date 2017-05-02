#include "asr_resp_handler.h"
#include "speech.pb.h"
#include "log.h"

#define WS_RECV_TIMEOUT 10

using std::shared_ptr;
using std::string;
using rokid::open::AsrResponse;
using rokid::open::SpeechErrorCode;

namespace rokid {
namespace speech {

void AsrRespHandler::start_handle(shared_ptr<AsrRespInfo> in, void* arg) {
}

int32_t AsrRespHandler::handle(shared_ptr<AsrRespInfo> in, void* arg) {
	AsrResponse resp;
	AsrCommonArgument* carg = (AsrCommonArgument*)arg;

	assert(arg);
	if (!in.get())
		return FLAG_ERROR;
	if (in->err != AsrError::ASR_SUCCESS)
		responses_.erase(in->id, in->err);

	if (in->status == 0) {
		in->status = 1;
		Log::d(tag__, "AsrRespHandler: %d, asr start", in->id);
		responses_.start(in->id);
		return FLAG_NOT_POLL_NEXT;
	} else if (in->status == 2) {
		Log::d(tag__, "AsrRespHandler: %d, asr end", in->id);
		responses_.end(in->id);
		return 0;
	}

	shared_ptr<SpeechConnection> conn = carg->keepalive_.get_conn(0);
	if (conn.get() == NULL) {
		responses_.erase(in->id, AsrError::ASR_SDK_CLOSED);
		return 0;
	}
	if (!conn->recv(resp, WS_RECV_TIMEOUT)) {
		Log::w(tag__, "AsrRespHandler: receive failed");
		responses_.erase(in->id, AsrError::ASR_SERVICE_UNAVAILABLE);
		carg->keepalive_.shutdown(conn.get());
		return 0;
	}
	if (resp.result() != SpeechErrorCode::SUCCESS) {
		responses_.erase(in->id, resp.result());
		return 0;
	}
	int32_t r = FLAG_NOT_POLL_NEXT | FLAG_BREAK_LOOP;
	if (resp.has_asr()) {
#ifdef LOW_PB_VERSION
		shared_ptr<string> asr(new string(resp.asr()));
#else
		shared_ptr<string> asr(resp.release_asr());
#endif
		responses_.stream(in->id, asr);
		Log::d(tag__, "AsrRespHandler: id %d, read asr %s", in->id,
				asr->c_str());
		r = FLAG_NOT_POLL_NEXT;
	}
	if (resp.finish())
		in->status = 2;
	return r;
}

void AsrRespHandler::end_handle(shared_ptr<AsrRespInfo> in, void* arg) {
}

shared_ptr<AsrResult> AsrRespHandler::poll() {
	uint32_t err;
	int32_t id;
	shared_ptr<string> asr;
	int32_t r = responses_.pop(id, asr, err);
	if (r < 0)
		return NULL;
	shared_ptr<AsrResult> res(new AsrResult());
	res->id = id;
	res->type = r;
	res->err = (AsrError)err;
	if (r == 0) {
		assert(asr.get());
		res->asr = *asr;
	}
	return res;
}

bool AsrRespHandler::closed() {
	return false;
}

} // namespace speech
} // namespace rokid
