#include "asr_impl.h"
#include "speech_connection.h"
#include "log.h"

#define WS_SEND_TIMEOUT 10000

using std::unique_ptr;
using std::shared_ptr;
using std::string;
using rokid::open::ReqType;
using rokid::open::AsrRequest;
using rokid::open::AsrResponse;

namespace rokid {
namespace speech {

AsrImpl::AsrImpl() : initialized_(false) {
}

bool AsrImpl::prepare() {
	if (initialized_)
		return true;
	next_id_ = 0;
	connection_.initialize(SOCKET_BUF_SIZE, &config_, "asr");
	initialized_ = true;
	req_thread_ = new thread([=] { send_reqs(); });
	resp_thread_ = new thread([=] { gen_results(); });
	return true;
}

void AsrImpl::release() {
	Log::d(tag__, "AsrImpl.release, initialized = %d", initialized_);
	if (initialized_) {
		// notify req thread to exit
		unique_lock<mutex> req_locker(req_mutex_);
		initialized_ = false;
		requests_.close();
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

int32_t AsrImpl::start() {
	if (!initialized_)
		return -1;
	lock_guard<mutex> locker(req_mutex_);
	int32_t id = next_id();
	if (!requests_.start(id))
		return -1;
	Log::d(tag__, "AsrImpl: start %d", id);
	req_cond_.notify_one();
	return id;
}

void AsrImpl::voice(int32_t id, const uint8_t* voice, uint32_t length) {
	if (!initialized_)
		return;
	if (id <= 0)
		return;
	if (voice == NULL || length == 0)
		return;
	lock_guard<mutex> locker(req_mutex_);
	shared_ptr<string> spv(new string((const char*)voice, length));
	if (requests_.stream(id, spv)) {
		Log::d(tag__, "AsrImpl: voice %d, len %u", id, length);
		req_cond_.notify_one();
	}
}

void AsrImpl::end(int32_t id) {
	if (!initialized_)
		return;
	if (id <= 0)
		return;
	lock_guard<mutex> locker(req_mutex_);
	if (requests_.end(id)) {
		Log::d(tag__, "AsrImpl: end %d", id);
		req_cond_.notify_one();
	}
}

void AsrImpl::cancel(int32_t id) {
	unique_lock<mutex> req_locker(req_mutex_);
	if (!initialized_)
		return;
	Log::d(tag__, "AsrImpl: cancel %d", id);
	if (id > 0) {
		if (requests_.erase(id))
			req_cond_.notify_one();
		else {
			req_locker.unlock();
			lock_guard<mutex> resp_locker(resp_mutex_);
			controller_.cancel_op(id, resp_cond_);
		}
	} else {
		int32_t min_id;
		requests_.clear(&min_id, NULL);
		if (min_id > 0)
			req_cond_.notify_one();
		req_locker.unlock();
		lock_guard<mutex> resp_locker(resp_mutex_);
		controller_.cancel_op(0, resp_cond_);
	}
}

void AsrImpl::config(const char* key, const char* value) {
	config_.set(key, value);
}

bool AsrImpl::poll(AsrResult& res) {
	unique_lock<mutex> locker(resp_mutex_);
	while (initialized_) {
		if (!responses_.empty()) {
			res = *responses_.front();
			responses_.pop_front();
			Log::d(tag__, "AsrImpl.poll return result %d", res.id);
			return true;
		}
		if (gen_result_by_status())
			continue;
		Log::d(tag__, "AsrImpl.poll wait");
		resp_cond_.wait(locker);
	}
	Log::d(tag__, "AsrImpl.poll return false");
	return false;
}

void AsrImpl::send_reqs() {
	int32_t r;
	int32_t id;
	shared_ptr<string> voice;
	uint32_t err;
	int32_t rv;

	Log::d(tag__, "thread 'send_reqs' begin");
	while (true) {
		unique_lock<mutex> locker(req_mutex_);
		if (!initialized_)
			break;
		r = requests_.pop(id, voice, err);
		if (r < 0) {
			Log::d(tag__, "AsrImpl.send_reqs wait req available");
			req_cond_.wait(locker);
		} else {
			locker.unlock();

			Log::d(tag__, "call do_request %d, %d, %d", id, r, err);
			rv = do_request(id, (uint32_t)r, voice, err);

			if (rv == 0) {
				Log::d(tag__, "AsrImpl.send_reqs wait op finish");
				unique_lock<mutex> resp_locker(resp_mutex_);
				controller_.wait_op_finish(resp_locker);
			}
		}
	}
	Log::d(tag__, "thread 'send_reqs' quit");
}

int32_t AsrImpl::do_request(int32_t id, uint32_t type,
		shared_ptr<string>& voice, uint32_t err) {
	AsrRequest treq;
	if (type == StreamQueue<string>::POP_TYPE_REMOVED) {
		Log::d(tag__, "do_request: cancelled req");
		unique_lock<mutex> locker(resp_mutex_);
		controller_.new_op(id, ASR_STATUS_CANCELLED);
		resp_cond_.notify_one();
		locker.unlock();
		return 0;
	}
	int32_t rv;
	if (type == StreamQueue<string>::POP_TYPE_START) {
		Log::d(tag__, "do_request: send asr start to server. (%d)", id);
		unique_lock<mutex> locker(resp_mutex_);
		controller_.new_op(id, ASR_STATUS_START);
		locker.unlock();

		treq.set_id(id);
		treq.set_type(ReqType::START);
		treq.set_lang(config_.get("lang", "zh"));
		treq.set_codec(config_.get("codec", "pcm"));
		treq.set_vt(config_.get("vt", ""));
		rv = 1;
	} else if (type == StreamQueue<string>::POP_TYPE_END) {
		Log::d(tag__, "do_request: send asr end to server. (%d)", id);
		treq.set_id(id);
		treq.set_type(ReqType::END);
		rv = 0;
	} else if (type == StreamQueue<string>::POP_TYPE_DATA) {
		Log::d(tag__, "do_request: send asr voice to server. (%d)", id);
		treq.set_id(id);
		treq.set_type(ReqType::VOICE);
		treq.set_voice(*voice);
		rv = 1;
	} else {
		Log::w(tag__, "AsrImpl.do_request: (%d) req type is %u, "
				"it's impossible!", id, type);
		assert(false);
		return -1;
	}

	int32_t r = connection_.send(treq, WS_SEND_TIMEOUT);
	if (r != CO_SUCCESS) {
		AsrError err = ASR_UNKNOWN;
		if (r == CO_CONNECTION_NOT_AVAILABLE)
			err = ASR_SERVICE_UNAVAILABLE;
		Log::d(tag__, "AsrImpl.do_request: (%d) send failed %d, "
				"set op error", id, r);
		lock_guard<mutex> locker(resp_mutex_);
		controller_.set_op_error(err, resp_cond_);
		return -1;
	}
	return rv;
}

void AsrImpl::gen_results() {
	AsrResponse resp;
	int32_t r;
	AsrError err;

	Log::d(tag__, "thread 'gen_results' run");
	while (true) {
		r = connection_.recv(resp);
		if (r == CO_NOT_READY)
			break;
		unique_lock<mutex> locker(resp_mutex_);
		if (r == CO_SUCCESS) {
			gen_result_by_resp(resp);
		} else if (r == CO_CONNECTION_BROKEN)
			controller_.set_op_error(ASR_SERVICE_UNAVAILABLE, resp_cond_);
		else
			controller_.set_op_error(ASR_UNKNOWN, resp_cond_);
		locker.unlock();
	}
	Log::d(tag__, "thread 'gen_results' quit");
}

void AsrImpl::gen_result_by_resp(AsrResponse& resp) {
	if (controller_.id_ == resp.id()) {
		shared_ptr<AsrResult> result;

		if (controller_.status_ == ASR_STATUS_START) {
			result.reset(new AsrResult());
			result->id = resp.id();
			result->type = ASR_RES_START;
			responses_.push_back(result);
			controller_.status_ = ASR_STATUS_STREAMING;
			Log::d(tag__, "gen_result_by_resp(%d): push start resp, "
					"Status Start --> Streaming", result->id);
		}
		if (!gen_result_by_status()) {
			Log::d(tag__, "AsrResponse has_asr(%d), finish(%d)",
					resp.has_asr(), resp.finish());
			assert(controller_.status_ == ASR_STATUS_STREAMING);
			if (resp.has_asr()) {
				result.reset(new AsrResult());
				result->id = resp.id();
				result->type = ASR_RES_ASR;
				result->asr = resp.asr();
				responses_.push_back(result);
				Log::d(tag__, "gen_result_by_resp(%d): push asr resp "
						"%s", result->id, result->asr.c_str());
			}

			if (resp.finish()) {
				result.reset(new AsrResult());
				result->id = resp.id();
				result->type = ASR_RES_END;
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

bool AsrImpl::gen_result_by_status() {
	shared_ptr<AsrResult> result;
	if (controller_.id_ == 0) {
		Log::d(tag__, "gen_result_by_status: Status End, return false");
		return false;
	}
	if (controller_.status_ == ASR_STATUS_CANCELLED) {
		result.reset(new AsrResult());
		result->id = controller_.id_;
		result->type = ASR_RES_CANCELLED;
		responses_.push_back(result);
		Log::d(tag__, "gen_result_by_status(%d) Status Cancelled --> "
				"End, push result", result->id);
		controller_.finish_op();
		return true;
	} else if (controller_.status_ == ASR_STATUS_ERROR) {
		result.reset(new AsrResult());
		result->id = controller_.id_;
		result->type = ASR_RES_ERROR;
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

Asr* new_asr() {
	return new AsrImpl();
}

void delete_asr(Asr* asr) {
	if (asr)
		delete asr;
}

} // namespace speech
} // namespace rokid
