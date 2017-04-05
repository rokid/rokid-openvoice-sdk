#include <chrono>
#include "speech_resp_handler.h"
#include "speech.pb.h"
#include "log.h"
#include "grpc++/impl/codegen/proto_utils.h"

using std::shared_ptr;
using std::lock_guard;
using std::unique_lock;
using std::mutex;
using rokid::open::SpeechResponse;
using grpc::Status;

namespace rokid {
namespace speech {

SpeechRespHandler::SpeechRespHandler() : start_response_(false), closed_(false) {
}

void SpeechRespHandler::start_handle(shared_ptr<SpeechRespInfo> in, void* arg) {
	assert(arg);
	if (in.get() && in->type == 1)
		start_response_ = true;
}

int32_t SpeechRespHandler::handle(shared_ptr<SpeechRespInfo> in, void* arg) {
	assert(arg);

	if (!in.get())
		return FLAG_ERROR;
	shared_ptr<SpeechResult> r(new SpeechResult());
	if (in->type == 0) {
		Log::d(tag__, "RespHandler: text req %d, resp\n\tasr: %s\n\tnlp: %s\n\taction: %s",
				r->id, r->asr.c_str(), r->nlp.c_str(), r->action.c_str());
		lock_guard<mutex> locker(mutex_);
		r->id = in->result.id;
		r->type = 1;
		r->err = 0;
		responses_.push_back(r);
		r.reset(new SpeechResult());
		*r = in->result;
		responses_.push_back(r);
		r.reset(new SpeechResult());
		r->id = in->result.id;
		r->type = 2;
		r->err = 0;
		responses_.push_back(r);
		cond_.notify_one();
		return 0;
	}

	CommonArgument* carg = (CommonArgument*)arg;
	assert(in->type == 1);
	if (start_response_) {
		start_response_ = false;
		r->id = carg->current_id;
		r->type = 1;
		r->err = 0;
		Log::d(tag__, "RespHandler: voice req %d, start resp", r->id);
		lock_guard<mutex> locker(mutex_);
		responses_.push_back(r);
		cond_.notify_one();
		return FLAG_NOT_POLL_NEXT;
	}

	SpeechResponse resp;
	if (in->stream->Read(&resp)) {
		r->id = carg->current_id;
		r->type = 0;
		r->err = 0;
		r->asr = resp.asr();
		r->nlp = resp.nlp();
		r->action = resp.action();
		Log::d(tag__, "RespHandler: voice req %d, data resp\n\tasr: %s\n\tnlp: %s\n\taction: %s",
				r->id, r->asr.c_str(), r->nlp.c_str(), r->action.c_str());
		lock_guard<mutex> locker(mutex_);
		responses_.push_back(r);
		cond_.notify_one();
		return FLAG_NOT_POLL_NEXT;
	}

	Status status = in->stream->Finish();
	if (status.ok()) {
		r->id = carg->current_id;
		r->type = 2;
		r->err = 0;
	} else {
		r->id = carg->current_id;
		r->type = 4;
		Log::d(tag__, "grpc Read failed, %d, %s",
				status.error_code(), status.error_message().c_str());
		if (status.error_code() == grpc::StatusCode::UNAVAILABLE)
			r->err = 1;
		else
			r->err = 2;
	}
	Log::d(tag__, "RespHandler: voice req %d, end resp, err %u", r->id, r->err);
	lock_guard<mutex> locker(mutex_);
	responses_.push_back(r);
	cond_.notify_one();
	return 0;
}

void SpeechRespHandler::end_handle(shared_ptr<SpeechRespInfo> in, void* arg) {
	CommonArgument* carg = (CommonArgument*)arg;
	carg->context.reset();
	carg->current_id = 0;
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

bool SpeechRespHandler::closed() {
	return closed_;
}

} // namespace speech
} // namespace rokid
