#include <chrono>
#include "tts_impl.h"

#define WS_SEND_TIMEOUT 10000

namespace rokid {
namespace speech {

using std::shared_ptr;
using std::thread;
using std::mutex;
using std::unique_lock;
using std::lock_guard;
using std::list;
using rokid::open::TtsRequest;
using rokid::open::TtsResponse;

TtsImpl::TtsImpl() : initialized_(false) {
}

bool TtsImpl::prepare() {
	if (initialized_)
		return true;
	next_id_ = 0;
	connection_.initialize(SOCKET_BUF_SIZE, &config_, "tts");
	initialized_ = true;
	req_thread_ = new thread([=] { send_reqs(); });
	resp_thread_ = new thread([=] { gen_results(); });
	return true;
}

void TtsImpl::release() {
	Log::d(tag__, "TtsImpl.release, initialized = %d", initialized_);
	if (initialized_) {
		// notify req thread to exit
		unique_lock<mutex> req_locker(req_mutex_);
		initialized_ = false;
		requests_.clear();
		req_cond_.notify_one();
		req_locker.unlock();
		req_thread_->join();
		delete req_thread_;

		connection_.release();

		// notify resp thread to exit
		unique_lock<mutex> resp_locker(resp_mutex_);
		responses_.clear();
		controller_.finish_op();
		resp_cond_.notify_one();
		resp_locker.unlock();
		resp_thread_->join();
		delete resp_thread_;
	}
}

int32_t TtsImpl::speak(const char* text) {
	if (!initialized_)
		return -1;
	Log::d(tag__, "speak %s", text);
	shared_ptr<TtsReqInfo> req(new TtsReqInfo());
	req->data = text;
	req->deleted = false;
	lock_guard<mutex> locker(req_mutex_);
	int32_t id = next_id();
	req->id = id;
	requests_.push_back(req);
	req_cond_.notify_one();
	return id;
}

void TtsImpl::cancel(int32_t id) {
	list<shared_ptr<TtsReqInfo> >::iterator it;
	bool erased = false;

	unique_lock<mutex> locker(req_mutex_);
	if (!initialized_)
		return;
	Log::d(tag__, "cancel %d", id);
	it = requests_.begin();
	while (it != requests_.end()) {
		if (id <= 0 || (*it)->id == id) {
			(*it)->deleted = true;
			erased = true;
		}
		++it;
	}
	locker.unlock();

	lock_guard<mutex> resp_locker(resp_mutex_);
	if (id <= 0)
		controller_.cancel_op(0, resp_cond_);
	else if (!erased)
		controller_.cancel_op(id, resp_cond_);
}

void TtsImpl::config(const char* key, const char* value) {
	config_.set(key, value);
}

bool TtsImpl::poll(TtsResult& res) {
	unique_lock<mutex> locker(resp_mutex_);
	while (initialized_) {
		if (!responses_.empty()) {
			res = *responses_.front();
			responses_.pop_front();
			Log::d(tag__, "TtsImpl.poll return result %d", res.id);
			return true;
		}
		if (gen_result_by_status())
			continue;
		Log::d(tag__, "TtsImpl.poll wait");
		resp_cond_.wait(locker);
	}
	Log::d(tag__, "TtsImpl.poll return false, sdk released");
	return false;
}

void TtsImpl::send_reqs() {
	shared_ptr<TtsReqInfo> req;
	Log::d(tag__, "thread 'send_reqs' begin");
	while (true) {
		unique_lock<mutex> locker(req_mutex_);
		if (!initialized_)
			break;
		if (requests_.empty()) {
			Log::d(tag__, "TtsImpl.send_reqs wait req available");
			req_cond_.wait(locker);
		} else {
			req = requests_.front();
			requests_.pop_front();
			locker.unlock();

			do_request(req);

			Log::d(tag__, "TtsImpl.send_reqs wait op finish");
			unique_lock<mutex> resp_locker(resp_mutex_);
			controller_.wait_op_finish(resp_locker);
		}
	}
	Log::d(tag__, "thread 'send_reqs' quit");
}

void TtsImpl::do_request(shared_ptr<TtsReqInfo> req) {
	unique_lock<mutex> locker(resp_mutex_);
	if (req->deleted) {
		Log::d(tag__, "do_request: cancelled req");
		controller_.new_op(req->id, TTS_STATUS_CANCELLED);
		resp_cond_.notify_one();
		locker.unlock();
	} else {
		Log::d(tag__, "do_request: send req to server. (%d:%s)",
				req->id, req->data.c_str());
		controller_.new_op(req->id, TTS_STATUS_START);
		locker.unlock();
		TtsRequest treq;
		treq.set_id(req->id);
		treq.set_text(req->data.c_str());
		treq.set_declaimer(config_.get("declaimer", "zh"));
		treq.set_codec(config_.get("codec", "pcm"));
		int32_t r = connection_.send(treq, WS_SEND_TIMEOUT);
		if (r != CO_SUCCESS) {
			TtsError err = TTS_UNKNOWN;
			if (r == CO_CONNECTION_NOT_AVAILABLE)
				err = TTS_SERVICE_UNAVAILABLE;
			Log::w(tag__, "do_request: (%d) send req failed %d, "
					"set op error", req->id, r);
			locker.lock();
			controller_.set_op_error(err, resp_cond_);
			locker.unlock();
		}
	}
}

void TtsImpl::gen_results() {
	TtsResponse resp;
	int32_t r;
	TtsError err;
	
	Log::d(tag__, "thread 'gen_results' run");
	while (true) {
		r = connection_.recv(resp);
		if (r == CO_NOT_READY)
			break;
		unique_lock<mutex> locker(resp_mutex_);
		if (r == CO_SUCCESS) {
			gen_result_by_resp(resp);
		} else if (r == CO_CONNECTION_BROKEN)
			controller_.set_op_error(TTS_SERVICE_UNAVAILABLE, resp_cond_);
		else
			controller_.set_op_error(TTS_UNKNOWN, resp_cond_);
		locker.unlock();
	}
	Log::d(tag__, "thread 'gen_results' quit");
}

void TtsImpl::gen_result_by_resp(TtsResponse& resp) {
	if (controller_.id_ == resp.id()) {
		shared_ptr<TtsResult> result;

		if (controller_.status_ == TTS_STATUS_START) {
			result.reset(new TtsResult());
			result->id = resp.id();
			result->type = TTS_RES_START;
			responses_.push_back(result);
			controller_.status_ = TTS_STATUS_STREAMING;
			Log::d(tag__, "gen_result_by_resp(%d): push start resp, "
					"Status Start --> Streaming", result->id);
		}
		if (!gen_result_by_status()) {
			Log::d(tag__, "TtsResponse has_voice(%d), finish(%d)",
					resp.has_voice(), resp.finish());
			assert(controller_.status_ == TTS_STATUS_STREAMING);
			if (resp.has_voice()) {
				result.reset(new TtsResult());
				result->id = resp.id();
				result->type = TTS_RES_VOICE;
#ifdef LOW_PB_VERSION
				result->voice.reset(new string(resp.voice()));
#else
				result->voice.reset(resp.release_voice());
#endif
				responses_.push_back(result);
				Log::d(tag__, "gen_result_by_resp(%d): push voice "
						"resp, %d bytes", result->id,
						result->voice->length());
			}

			if (resp.finish()) {
				result.reset(new TtsResult());
				result->id = resp.id();
				result->type = TTS_RES_END;
				responses_.push_back(result);
				Log::d(tag__, "gen_result_by_resp(%d): push end resp, "
						"Status Streaming --> End", result->id);
				controller_.finish_op();
			}
		}

		if (!responses_.empty()) {
			resp_cond_.notify_one();
		}
	}
}

bool TtsImpl::gen_result_by_status() {
	shared_ptr<TtsResult> result;
	if (controller_.id_ == 0) {
		Log::d(tag__, "gen_result_by_status: Status End, return false");
		return false;
	}
	if (controller_.status_ == TTS_STATUS_CANCELLED) {
		result.reset(new TtsResult());
		result->id = controller_.id_;
		result->type = TTS_RES_CANCELLED;
		responses_.push_back(result);
		Log::d(tag__, "gen_result_by_status(%d) Status Cancelled --> "
				"End, push result", result->id);
		controller_.finish_op();
		return true;
	} else if (controller_.status_ == TTS_STATUS_ERROR) {
		result.reset(new TtsResult());
		result->id = controller_.id_;
		result->type = TTS_RES_ERROR;
		result->err = controller_.error_;
		responses_.push_back(result);
		Log::d(tag__, "gen_result_by_status(%d) Status Error --> "
				"End, push result", result->id);
		controller_.finish_op();
		return true;
	}
	Log::d(tag__, "gen_result_by_status(%d) Other Status, "
			"return false", controller_.id_);
	return false;
}

Tts* new_tts() {
	return new TtsImpl();
}

void delete_tts(Tts* tts) {
	if (tts)
		delete tts;
}

} // namespace speech
} // namespace rokid
