#include <assert.h>
#include <chrono>
#include "speech.pb.h"
#include "asr_req_handler.h"
#include "log.h"

#define WS_SEND_TIMEOUT 5

using std::shared_ptr;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using rokid::open::AsrRequest;
using rokid::open::ReqType;

namespace rokid {
namespace speech {

AsrReqHandler::AsrReqHandler() : closed_(false) {
}

shared_ptr<AsrRespInfo> AsrReqHandler::poll() {
	unique_lock<mutex> locker(mutex_);
	if (resp_streams_.empty()) {
		cond_.wait(locker);
		if (closed_)
			return NULL;
		assert(!resp_streams_.empty());
	}
	shared_ptr<AsrRespInfo> stream = resp_streams_.front();
	assert(stream.get());
	resp_streams_.pop_front();
	return stream;
}

void AsrReqHandler::close() {
	lock_guard<mutex> locker(mutex_);
	closed_ = true;
	cond_.notify_one();
}

void AsrReqHandler::reset() {
	closed_ = false;
}

bool AsrReqHandler::closed() {
	return closed_;
}

void AsrReqHandler::start_handle(shared_ptr<AsrReqInfo> in, void* arg) {
}

int32_t AsrReqHandler::handle(shared_ptr<AsrReqInfo> in, void* arg) {
	AsrCommonArgument* carg = (AsrCommonArgument*)arg;
	AsrRequest req;

	Log::d(tag__, "AsrReqHandler: handle req type %u", in->type);
	assert(arg);
	if (!in.get())
		return FLAG_ERROR;
	switch (in->type) {
		case 0: {
			shared_ptr<SpeechConnection> conn = carg->keepalive_.get_conn(0);
			if (conn.get() == NULL)
				return FLAG_ERROR;
			req.set_id(in->id);
			req.set_type(ReqType::VOICE);
			req.set_voice(*in->voice);
			if (!conn->send(req, WS_SEND_TIMEOUT)) {
				Log::w(tag__, "AsrReqHandler: send asr voice failed");
				carg->keepalive_.shutdown(conn.get());
				return FLAG_ERROR;
			}
			Log::d(tag__, "AsrReqHandler: send asr voice %u bytes",
					in->voice->length());
			break;
		}
		case 1: {
			shared_ptr<SpeechConnection> conn = carg->keepalive_.get_conn(0);
			if (conn.get() == NULL) {
				put_response(in->id, AsrError::ASR_SDK_CLOSED);
				break;
			}
			req.set_id(in->id);
			req.set_type(ReqType::START);
			req.set_lang(carg->config.get("lang", "zh"));
			req.set_codec(carg->config.get("codec", "pcm"));
			req.set_vt(carg->config.get("vt", ""));
			if (!conn->send(req, WS_SEND_TIMEOUT)) {
				Log::w(tag__, "AsrReqHandler: send asr begin failed");
				put_response(in->id, AsrError::ASR_SERVICE_UNAVAILABLE);
				carg->keepalive_.shutdown(conn.get());
				break;
			}
			Log::d(tag__, "AsrReqHandler: asr begin, id is %d", in->id);
			put_response(in->id, AsrError::ASR_SUCCESS);
			break;
		}
		case 2: {
			shared_ptr<SpeechConnection> conn = carg->keepalive_.get_conn(0);
			if (conn.get() == NULL)
				return FLAG_ERROR;
			req.set_id(in->id);
			req.set_type(ReqType::END);
			if (!conn->send(req, WS_SEND_TIMEOUT)) {
				Log::w(tag__, "AsrReqHandler: send asr end failed");
				carg->keepalive_.shutdown(conn.get());
				return FLAG_ERROR;
			}
			Log::d(tag__, "AsrReqHandler: asr end, id is %d", in->id);
			break;
		}
		case 3: {
			cancel_handler_->cancel(in->id);
			shared_ptr<SpeechConnection> conn = carg->keepalive_.get_conn(0);
			if (conn.get() == NULL)
				return FLAG_ERROR;
			req.set_id(in->id);
			req.set_type(ReqType::END);
			if (!conn->send(req, WS_SEND_TIMEOUT)) {
				Log::w(tag__, "AsrReqHandler: cancel, send asr end failed");
				carg->keepalive_.shutdown(conn.get());
				return FLAG_ERROR;
			}
			cancel_handler_->cancelled(in->id);
			Log::d(tag__, "AsrReqHandler: asr cancel, id is %d", in->id);
			return FLAG_BREAK_LOOP;
		}
		default:
			// unkown error, impossible happens
			error_code_ = -2;
			Log::d(tag__, "AsrReqHandler: asr unknow error, req type is %u",
					in->type);
			return FLAG_ERROR;
	}
	return 0;
}

void AsrReqHandler::end_handle(shared_ptr<AsrReqInfo> in, void* arg) {
	// do nothing
}

void AsrReqHandler::put_response(int32_t id, AsrError err) {
	shared_ptr<AsrRespInfo> resp(new AsrRespInfo());
	resp->id = id;
	resp->err = err;
	resp->status = 0;
	lock_guard<mutex> locker(mutex_);
	resp_streams_.push_back(resp);
	cond_.notify_one();
}

} // namespace speech
} // namespace rokid
