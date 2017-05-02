#include <chrono>
#include "speech_resp_handler.h"
#include "speech.pb.h"
#include "log.h"

#define WS_RECV_TIMEOUT 10

using std::shared_ptr;
using std::lock_guard;
using std::unique_lock;
using std::mutex;
using rokid::open::SpeechResponse;
using rokid::open::SpeechErrorCode;

namespace rokid {
namespace speech {

SpeechRespHandler::SpeechRespHandler() : closed_(false) {
}

void SpeechRespHandler::start_handle(shared_ptr<SpeechRespInfo> in, void* arg) {
}

int32_t SpeechRespHandler::handle(shared_ptr<SpeechRespInfo> in, void* arg) {
	assert(arg);
	SpeechCommonArgument* carg = (SpeechCommonArgument*)arg;

	if (!in.get())
		return FLAG_ERROR;
	shared_ptr<SpeechResult> r(new SpeechResult());
	if (in->err != SpeechError::SPEECH_SUCCESS) {
		r->id = in->id;
		r->type = 4;
		r->err = in->err;
		put_response(r);
		return 0;
	}
	if (in->status == 0) {
		in->status = 1;
		r->id = in->id;
		r->type = 1;
		r->err = SpeechError::SPEECH_SUCCESS;
		put_response(r);
		return FLAG_NOT_POLL_NEXT;
	} else if (in->status == 2) {
		r->id = in->id;
		r->type = 2;
		r->err = SpeechError::SPEECH_SUCCESS;
		put_response(r);
		return 0;
	}

	shared_ptr<SpeechConnection> conn = carg->keepalive_.get_conn(0);
	if (conn.get() == NULL) {
		r->id = in->id;
		r->type = 4;
		r->err = SpeechError::SPEECH_SDK_CLOSED;
		put_response(r);
		return 0;
	}
	SpeechResponse resp;
	if (!conn->recv(resp, WS_RECV_TIMEOUT)) {
		r->id = in->id;
		r->type = 4;
		r->err = SpeechError::SPEECH_SERVICE_UNAVAILABLE;
		put_response(r);
		return 0;
	}
	if (resp.result() != SpeechErrorCode::SUCCESS) {
		r->id = in->id;
		r->type = 4;
		r->err = (SpeechError)resp.result();
		put_response(r);
		return 0;
	}
	assert(resp.id() == in->id);
	r->id = resp.id();
	r->type = 0;
	r->err = SpeechError::SPEECH_SUCCESS;
	r->asr = resp.asr();
	r->nlp = resp.nlp();
	r->action = resp.action();
	put_response(r);
	if (resp.has_finish() && resp.finish())
		in->status = 2;
	return FLAG_NOT_POLL_NEXT;
}

void SpeechRespHandler::end_handle(shared_ptr<SpeechRespInfo> in, void* arg) {
}

shared_ptr<SpeechResult> SpeechRespHandler::poll() {
	unique_lock<mutex> locker(mutex_);
	while (!closed_) {
		if (!responses_.empty()) {
			shared_ptr<SpeechResult> tmp = responses_.front();
			responses_.pop_front();
			return tmp;
		}
		cond_.wait(locker);
	}
	return NULL;
}

void SpeechRespHandler::close() {
	lock_guard<mutex> locker(mutex_);
	closed_ = true;
	cond_.notify_one();
}

void SpeechRespHandler::reset() {
	closed_ = false;
}

bool SpeechRespHandler::closed() {
	return closed_;
}

void SpeechRespHandler::put_response(shared_ptr<SpeechResult> r) {
	lock_guard<mutex> locker(mutex_);
	responses_.push_back(r);
	cond_.notify_one();
}

} // namespace speech
} // namespace rokid
