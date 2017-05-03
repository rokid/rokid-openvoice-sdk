#include <chrono>
#include "speech_req_provider.h"

using std::shared_ptr;
using std::string;
using std::mutex;
using std::unique_lock;
using std::lock_guard;

namespace rokid {
namespace speech {

SpeechReqProvider::SpeechReqProvider() : closed_(false) {
}

shared_ptr<SpeechReqInfo> SpeechReqProvider::poll() {
	unique_lock<mutex> locker(mutex_);
	while (!closed_) {
		if (!text_reqs_.empty()) {
			shared_ptr<SpeechReqInfo> tmp = text_reqs_.front();
			text_reqs_.pop_front();
			Log::d(tag__, "ReqProvider: poll text req %d, %s",
					tmp->id, tmp->data->c_str());
			return tmp;
		}

		int32_t id;
		shared_ptr<string> voice;
		uint32_t err;
		int32_t r = voice_reqs_.pop(id, voice, err);
		if (r >= 0) {
			shared_ptr<SpeechReqInfo> tmp(new SpeechReqInfo());
			switch (r) {
				case 0:
					tmp->type = 4;
					break;
				case 1:
				case 2:
				case 3:
					tmp->type = r;
					break;
				default:
					assert(false);
					break;
			}
			tmp->id = id;
			tmp->data = voice;
			Log::d(tag__, "ReqProvider: poll voice req %d, %u",
					tmp->id, tmp->type);
			return tmp;
		}

		cond_.wait(locker);
	}
	Log::d(tag__, "ReqProvider: closed, poll null pointer");
	return NULL;
}

void SpeechReqProvider::close() {
	lock_guard<mutex> locker(mutex_);
	closed_ = true;
	cond_.notify_one();
}

void SpeechReqProvider::reset() {
	closed_ = false;
}

bool SpeechReqProvider::closed() {
	return closed_;
}

void SpeechReqProvider::put_text(int32_t id, const char* text) {
	lock_guard<mutex> locker(mutex_);
	shared_ptr<SpeechReqInfo> p(new SpeechReqInfo());
	p->id = id;
	p->type = 0;
	p->data.reset(new string(text));
	text_reqs_.push_back(p);
	cond_.notify_one();
}

void SpeechReqProvider::start_voice(int32_t id) {
	lock_guard<mutex> locker(mutex_);
	voice_reqs_.start(id);
	cond_.notify_one();

}

void SpeechReqProvider::put_voice(int32_t id, const uint8_t* data, uint32_t length) {
	lock_guard<mutex> locker(mutex_);
	shared_ptr<string> v(new string((const char*)data, (size_t)length));
	voice_reqs_.stream(id, v);
	cond_.notify_one();
}

void SpeechReqProvider::end_voice(int32_t id) {
	lock_guard<mutex> locker(mutex_);
	voice_reqs_.end(id);
	cond_.notify_one();
}

void SpeechReqProvider::cancel(int32_t id) {
	lock_guard<mutex> locker(mutex_);
	list<shared_ptr<SpeechReqInfo> >::iterator it;
	if (id > 0) {
		for (it = text_reqs_.begin(); it != text_reqs_.end(); ++it) {
			if ((*it)->id == id) {
				text_reqs_.erase(it);
				cancel_handler_->cancelled(id);
				cond_.notify_one();
				return;
			}
		}
		if (voice_reqs_.erase(id)) {
			cond_.notify_one();
			return;
		}
		cancel_handler_->cancel(id);
	} else {
		bool need_notify = false;
		for (it = text_reqs_.begin(); it != text_reqs_.end(); ++it)
			cancel_handler_->cancelled((*it)->id);
		need_notify = !text_reqs_.empty();
		text_reqs_.clear();
		int32_t min_id;
		voice_reqs_.clear(&min_id, NULL);
		if (min_id > 1) {
			cancel_handler_->cancel(min_id - 1);
			need_notify = true;
		}
		if (need_notify)
			cond_.notify_one();
	}
}

} // namespace speech
} // namespace rokid
