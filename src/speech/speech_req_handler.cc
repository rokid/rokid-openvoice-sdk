#include <chrono>
#include "speech_req_handler.h"
#include "speech.pb.h"
#include "log.h"

#define WS_SEND_TIMEOUT 5

using std::shared_ptr;
using std::unique_ptr;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using rokid::open::SpeechResponse;
using rokid::open::SpeechRequest;
using rokid::open::ReqType;

namespace rokid {
namespace speech {

SpeechReqHandler::SpeechReqHandler() : closed_(false) {
}

void SpeechReqHandler::start_handle(shared_ptr<SpeechReqInfo> in, void* arg) {
}

static void config_req(SpeechRequest* req, SpeechConfig* config) {
	req->set_lang(config->get("lang", "zh"));
	req->set_vt(config->get("vt", ""));
	req->set_stack(config->get("stack", ""));
	req->set_device(config->get("device", ""));
}

static bool send_voice_end(SpeechCommonArgument* carg, int32_t id) {
	SpeechRequest req;
	shared_ptr<SpeechConnection> conn = carg->keepalive_.get_conn(0);

	if (conn.get() == NULL)
		return false;
	req.set_id(id);
	req.set_type(ReqType::END);
	if (!conn->send(req, WS_SEND_TIMEOUT)) {
		Log::w(tag__, "SpeechReqHandler: send voice end for id "
				"%d failed", id);
		carg->keepalive_.shutdown(conn.get());
		return false;
	}
	Log::d(tag__, "SpeechReqHandler: send voice end for id "
			"%d success", id);
	return true;
}

int32_t SpeechReqHandler::handle(shared_ptr<SpeechReqInfo> in, void* arg) {
	assert(arg);

	Log::d(tag__, "SpeechReqHandler.handle: arg = %p, in = %p",
			arg, in.get());
	if (in.get() == NULL)
		return FLAG_BREAK_LOOP;
	SpeechCommonArgument* carg = (SpeechCommonArgument*)arg;
	switch (in->type) {
		case 0: {
			SpeechRequest req;
			SpeechResponse resp;
			shared_ptr<SpeechConnection> conn = carg->keepalive_.get_conn(0);

			if (conn.get() == NULL) {
				put_response(in->id, SpeechError::SPEECH_SDK_CLOSED);
				break;
			}
			req.set_id(in->id);
			req.set_type(ReqType::TEXT);
			req.set_asr(*in->data);
			config_req(&req, &carg->config);
			if (!conn->send(req, WS_SEND_TIMEOUT)) {
				put_response(in->id, SpeechError::SPEECH_SERVICE_UNAVAILABLE);
				carg->keepalive_.shutdown(conn.get());
				break;
			}
			put_response(in->id, SpeechError::SPEECH_SUCCESS);
			break;
		}
		case 1: {
			SpeechRequest req;
			shared_ptr<SpeechConnection> conn = carg->keepalive_.get_conn(0);

			if (conn.get() == NULL) {
				put_response(in->id, SpeechError::SPEECH_SDK_CLOSED);
				break;
			}
			req.set_id(in->id);
			req.set_type(ReqType::START);
			req.set_codec(carg->config.get("codec", "pcm"));
			config_req(&req, &carg->config);
			if (!conn->send(req, WS_SEND_TIMEOUT)) {
				put_response(in->id, SpeechError::SPEECH_SERVICE_UNAVAILABLE);
				carg->keepalive_.shutdown(conn.get());
				break;
			}
			put_response(in->id, SpeechError::SPEECH_SUCCESS);
			break;
		}
		case 2: {
			if (!send_voice_end(carg, in->id))
				return FLAG_ERROR;
			break;
		}
		case 3: {
			Log::d(tag__, "ReqHandler: cancel %d", in->id);
			cancel_handler_->cancel(in->id);
			if (!send_voice_end(carg, in->id))
				return FLAG_ERROR;
			break;
		}
		case 4: {
			SpeechRequest req;
			shared_ptr<SpeechConnection> conn = carg->keepalive_.get_conn(0);

			if (conn.get() == NULL)
				return FLAG_ERROR;
			req.set_id(in->id);
			req.set_type(ReqType::VOICE);
			req.set_voice(*in->data);
			if (!conn->send(req, WS_SEND_TIMEOUT)) {
				Log::w(tag__, "SpeechReqHandler: send voice data for "
						"id %d failed", in->id);
				carg->keepalive_.shutdown(conn.get());
				return FLAG_ERROR;
			}
			Log::d(tag__, "SpeechReqHandler: send voice data for "
					"id %d success", in->id);
			break;
		}
		default: {
			assert(false);
		}
	}
	return 0;
}

void SpeechReqHandler::end_handle(shared_ptr<SpeechReqInfo> in, void* arg) {
}

shared_ptr<SpeechRespInfo> SpeechReqHandler::poll() {
	unique_lock<mutex> locker(mutex_);

	while(!closed_) {
		if (!responses_.empty()) {
			shared_ptr<SpeechRespInfo> tmp = responses_.front();
			responses_.pop_front();
			Log::d(tag__, "req handler poll %p", tmp.get());
			return tmp;
		}
		cond_.wait(locker);
	}
	Log::d(tag__, "req handler poll null, closed %d", closed_);
	return NULL;
}

void SpeechReqHandler::close() {
	lock_guard<mutex> locker(mutex_);
	closed_ = true;
	cond_.notify_one();
}

void SpeechReqHandler::reset() {
	closed_ = false;
}

bool SpeechReqHandler::closed() {
	return closed_;
}

void SpeechReqHandler::put_response(int32_t id, SpeechError err) {
	shared_ptr<SpeechRespInfo> resp(new SpeechRespInfo());
	resp->id = id;
	resp->err = err;
	resp->status = 0;
	lock_guard<mutex> locker(mutex_);
	responses_.push_back(resp);
	cond_.notify_one();
}

} // namespace speech
} // namespace rokid
