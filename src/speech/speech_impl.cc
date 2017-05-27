#include <chrono>
#include "speech_impl.h"

#define WS_SEND_TIMEOUT 10000

using std::shared_ptr;
using std::mutex;
using std::lock_guard;
using rokid::open::SpeechRequest;
using rokid::open::SpeechResponse;
using rokid::open::ReqType;

namespace rokid {
namespace speech {

SpeechImpl::SpeechImpl() : initialized_(false) {
}

bool SpeechImpl::prepare() {
	if (initialized_)
		return true;
	next_id_ = 0;
	connection_.initialize(SOCKET_BUF_SIZE, &config_, "speech");
	initialized_ = true;
	req_thread_ = new thread([=] { send_reqs(); });
	resp_thread_ = new thread([=] { gen_results(); });
	return true;
}

void SpeechImpl::release() {
	Log::d(tag__, "SpeechImpl.release, initialized = %d", initialized_);
	if (initialized_) {
		// notify req thread to exit
		unique_lock<mutex> req_locker(req_mutex_);
		initialized_ = false;
		voice_reqs_.close();
		text_reqs_.clear();
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

int32_t SpeechImpl::put_text(const char* text) {
	if (!initialized_ || text == NULL)
		return -1;
	int32_t id = next_id();
	lock_guard<mutex> locker(req_mutex_);
	shared_ptr<SpeechReqInfo> p(new SpeechReqInfo());
	p->id = id;
	p->type = SpeechReqType::TEXT;
	p->data.reset(new string(text));
	text_reqs_.push_back(p);
	Log::d(tag__, "put text %d, %s", id, text);
	req_cond_.notify_one();
	return id;
}

int32_t SpeechImpl::start_voice() {
	if (!initialized_)
		return -1;
	lock_guard<mutex> locker(req_mutex_);
	int32_t id = next_id();
	if (!voice_reqs_.start(id))
		return -1;
	Log::d(tag__, "start voice %d", id);
	req_cond_.notify_one();
	return id;
}

void SpeechImpl::put_voice(int32_t id, const uint8_t* voice, uint32_t length) {
	if (!initialized_)
		return;
	if (id <= 0 || voice == NULL || length == 0)
		return;
	lock_guard<mutex> locker(req_mutex_);
	shared_ptr<string> spv(new string((const char*)voice, length));
	if (voice_reqs_.stream(id, spv)) {
		Log::d(tag__, "put voice %d, len %u", id, length);
		req_cond_.notify_one();
	}
}

void SpeechImpl::end_voice(int32_t id) {
	if (!initialized_)
		return;
	if (id <= 0)
		return;
	lock_guard<mutex> locker(req_mutex_);
	if (voice_reqs_.end(id)) {
		Log::d(tag__, "end voice %d", id);
		req_cond_.notify_one();
	}
}

void SpeechImpl::cancel(int32_t id) {
	unique_lock<mutex> req_locker(req_mutex_);
	if (!initialized_)
		return;
	Log::d(tag__, "cancel %d", id);
	if (id > 0) {
		if (voice_reqs_.erase(id)) {
			req_cond_.notify_one();
			return;
		}
		list<shared_ptr<SpeechReqInfo> >::iterator it;
		for (it = text_reqs_.begin(); it != text_reqs_.end(); ++it) {
			if ((*it)->id == id) {
				(*it)->type = SpeechReqType::CANCELLED;
				return;
			}
		}
		req_locker.unlock();
		lock_guard<mutex> resp_locker(resp_mutex_);
		controller_.cancel_op(id, resp_cond_);
	} else {
		int32_t min_id;
		voice_reqs_.clear(&min_id, NULL);
		if (min_id > 0)
			req_cond_.notify_one();
		list<shared_ptr<SpeechReqInfo> >::iterator it;
		for (it = text_reqs_.begin(); it != text_reqs_.end(); ++it) {
			(*it)->type = SpeechReqType::CANCELLED;
		}
		req_locker.unlock();
		lock_guard<mutex> resp_locker(resp_mutex_);
		controller_.cancel_op(0, resp_cond_);
	}
}

void SpeechImpl::config(const char* key, const char* value) {
	config_.set(key, value);
}

bool SpeechImpl::poll(SpeechResult& res) {
	unique_lock<mutex> locker(resp_mutex_);
	while (initialized_) {
		if (!responses_.empty()) {
			res = *responses_.front();
			responses_.pop_front();
			Log::d(tag__, "SpeechImpl.poll return result %d", res.id);
			return true;
		}
		if (gen_result_by_status())
			continue;
		Log::d(tag__, "SpeechImpl.poll wait");
		resp_cond_.wait(locker);
	}
	Log::d(tag__, "SpeechImpl.poll return false;");
	return false;
}

static SpeechReqType sqtype_to_reqtype(int32_t type) {
	static SpeechReqType _tps[] = {
		SpeechReqType::VOICE_DATA,
		SpeechReqType::VOICE_START,
		SpeechReqType::VOICE_END,
		SpeechReqType::CANCELLED
	};
	assert(type >= 0 && type < sizeof(_tps)/sizeof(SpeechReqType));
	return _tps[type];
}

void SpeechImpl::send_reqs() {
	int32_t r;
	int32_t id;
	shared_ptr<string> voice;
	uint32_t err;
	int32_t rv;
	shared_ptr<SpeechReqInfo> info;

	Log::d(tag__, "thread 'send_reqs' begin");
	while (true) {
		unique_lock<mutex> locker(req_mutex_);
		if (!initialized_)
			break;
		r = voice_reqs_.pop(id, voice, err);
		if (r >= 0) {
			info.reset(new SpeechReqInfo());
			info->id = id;
			info->type = sqtype_to_reqtype(r);
			info->data = voice;
		} else if (!text_reqs_.empty()) {
			info = text_reqs_.front();
			text_reqs_.pop_front();
		} else {
			Log::d(tag__, "SpeechImpl.send_reqs wait req available");
			req_cond_.wait(locker);
			continue;
		}
		locker.unlock();

		rv = do_request(info);
		if (rv == 0) {
			Log::d(tag__, "SpeechImpl.send_reqs wait op finish");
			unique_lock<mutex> resp_locker(resp_mutex_);
			controller_.wait_op_finish(resp_locker);
		}
	}
	Log::d(tag__, "thread 'send_reqs' quit");
}

static void req_config(SpeechRequest& req, SpeechConfig& config) {
	req.set_lang(config.get("lang", "zh"));
	req.set_codec(config.get("codec", "pcm"));
	req.set_vt(config.get("vt", ""));
	req.set_stack(config.get("stack", ""));
	req.set_device(config.get("device", ""));
}

int32_t SpeechImpl::do_request(shared_ptr<SpeechReqInfo>& req) {
	SpeechRequest treq;
	int32_t rv = 1;
	switch (req->type) {
		case SpeechReqType::TEXT: {
			treq.set_id(req->id);
			treq.set_type(ReqType::TEXT);
			treq.set_asr(*req->data);
			req_config(treq, config_);
			rv = 0;

			Log::d(tag__, "SpeechImpl.do_request (%d) send text req",
					req->id);
			lock_guard<mutex> locker(resp_mutex_);
			controller_.new_op(req->id, SpeechStatus::START);
			break;
		}
		case SpeechReqType::VOICE_START: {
			treq.set_id(req->id);
			treq.set_type(ReqType::START);
			req_config(treq, config_);

			Log::d(tag__, "SpeechImpl.do_request (%d) send voice start",
					req->id);
			lock_guard<mutex> locker(resp_mutex_);
			controller_.new_op(req->id, SpeechStatus::START);
			break;
		}	
		case SpeechReqType::VOICE_END:
			treq.set_id(req->id);
			treq.set_type(ReqType::END);
			rv = 0;
			Log::d(tag__, "SpeechImpl.do_request (%d) send voice end",
					req->id);
			break;
		case SpeechReqType::CANCELLED: {
			lock_guard<mutex> locker(resp_mutex_);
			Log::d(tag__, "SpeechImpl.do_request (%d) cancel req",
					req->id);
			controller_.new_op(req->id, SpeechStatus::CANCELLED);
			resp_cond_.notify_one();
			return 0;
		}
		case SpeechReqType::VOICE_DATA:
			treq.set_id(req->id);
			treq.set_type(ReqType::END);
			treq.set_voice(*req->data);
			Log::d(tag__, "SpeechImpl.do_request (%d) send voice data",
					req->id);
			break;
		default:
			Log::w(tag__, "SpeechImpl.do_request: (%d) req type is %u, "
					"it's impossible!", req->id, req->type);
			assert(false);
			return -1;
	}

	int32_t r = connection_.send(treq, WS_SEND_TIMEOUT);
	if (r != CO_SUCCESS) {
		SpeechError err = SPEECH_UNKNOWN;
		if (r == CO_CONNECTION_NOT_AVAILABLE)
			err = SPEECH_SERVICE_UNAVAILABLE;
		Log::w(tag__, "SpeechImpl.do_request: (%d) send req failed "
				"%d, set op error", req->id, r);
		lock_guard<mutex> locker(resp_mutex_);
		controller_.set_op_error(err, resp_cond_);
		return -1;
	}
	return rv;
}

void SpeechImpl::gen_results() {
	SpeechResponse resp;
	int32_t r;
	SpeechError err;

	Log::d(tag__, "thread 'gen_results' run");
	while (true) {
		r = connection_.recv(resp);
		if (r == CO_NOT_READY)
			break;
		unique_lock<mutex> locker(resp_mutex_);
		if (r == CO_SUCCESS) {
			gen_result_by_resp(resp);
		} else if (r == CO_CONNECTION_BROKEN)
			controller_.set_op_error(SPEECH_SERVICE_UNAVAILABLE, resp_cond_);
		else
			controller_.set_op_error(SPEECH_UNKNOWN, resp_cond_);
		locker.unlock();
	}
	Log::d(tag__, "thread 'gen_results' quit");
}

void SpeechImpl::gen_result_by_resp(SpeechResponse& resp) {
	if (controller_.id_ == resp.id()) {
		shared_ptr<SpeechResult> result;

		if (controller_.status_ == SpeechStatus::START) {
			result.reset(new SpeechResult());
			result->id = resp.id();
			result->type = SPEECH_RES_START;
			responses_.push_back(result);
			controller_.status_ = SpeechStatus::STREAMING;
			Log::d(tag__, "gen_result_by_resp(%d): push start resp, "
					"Status Start --> Streaming", result->id);
		}
		if (!gen_result_by_status()) {
			Log::d(tag__, "SpeechResponse finish(%d)", resp.finish());
			assert(controller_.status_ == SpeechStatus::STREAMING);
			result.reset(new SpeechResult());
			result->id = resp.id();
			result->type = SPEECH_RES_NLP;
			result->asr = resp.asr();
			result->nlp = resp.nlp();
			result->action = resp.action();
			responses_.push_back(result);
			Log::d(tag__, "gen_result_by_resp(%d): push nlp resp "
					"%s", result->id, result->action.c_str());

			if (resp.finish()) {
				result.reset(new SpeechResult());
				result->id = resp.id();
				result->type = SPEECH_RES_END;
				responses_.push_back(result);
				Log::d(tag__, "gen_result_by_resp(%d): push end resp, "
						"Status Streaming --> End", result->id);
				controller_.finish_op();
			}
		}

		if (!responses_.empty()) {
			Log::d(tag__, "some responses put to queue, "
					"awake poll thread");
			resp_cond_.notify_one();
		}
	}
}

bool SpeechImpl::gen_result_by_status() {
	if (controller_.id_ == 0) {
		Log::d(tag__, "gen_result_by_status: Status End, return false");
		return false;
	}
	shared_ptr<SpeechResult> result;
	if (controller_.status_ == SpeechStatus::CANCELLED) {
		result.reset(new SpeechResult());
		result->id = controller_.id_;
		result->type = SPEECH_RES_CANCELLED;
		responses_.push_back(result);
		Log::d(tag__, "gen_result_by_status(%d) Status Cancelled --> "
				"End, push result", result->id);
		controller_.finish_op();
		return true;
	} else if (controller_.status_ == SpeechStatus::ERROR) {
		result.reset(new SpeechResult());
		result->id = controller_.id_;
		result->type = SPEECH_RES_ERROR;
		result->err = controller_.error_;
		responses_.push_back(result);
		Log::d(tag__, "gen_result_by_status(%d) Status Error --> End, "
				"push result", result->id);
		controller_.finish_op();
		return true;
	}
	Log::d(tag__, "gen_result_by_status(%d) Other Status, return false",
			controller_.id_);
	return false;
}

Speech* new_speech() {
	return new SpeechImpl();
}

void delete_speech(Speech* speech) {
	if (speech) {
		delete speech;
	}
}

} // namespace speech
} // namespace rokid
