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
		connection_.release();
		requests_.close();
		req_cond_.notify_one();
		req_locker.unlock();
		req_thread_->join();
		delete req_thread_;

		// notify resp thread to exit
		unique_lock<mutex> resp_locker(resp_mutex_);
		responses_.close();
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
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(tag__, "AsrImpl: start %d", id);
#endif
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
#ifdef SPEECH_SDK_DETAIL_TRACE
		Log::d(tag__, "AsrImpl: voice %d, len %u", id, length);
#endif
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
#ifdef SPEECH_SDK_DETAIL_TRACE
		Log::d(tag__, "AsrImpl: end %d", id);
#endif
		req_cond_.notify_one();
	}
}

void AsrImpl::cancel(int32_t id) {
	lock_guard<mutex> req_locker(req_mutex_);
	if (!initialized_)
		return;
	Log::d(tag__, "AsrImpl: cancel %d", id);
	if (id > 0) {
		if (requests_.erase(id))
			req_cond_.notify_one();
		else {
			lock_guard<mutex> resp_locker(resp_mutex_);
			controller_.cancel_op(id, resp_cond_);
		}
	} else {
		int32_t min_id;
		requests_.clear(&min_id, NULL);
		if (min_id > 0)
			req_cond_.notify_one();
		lock_guard<mutex> resp_locker(resp_mutex_);
		controller_.cancel_op(0, resp_cond_);
	}
}

void AsrImpl::config(const char* key, const char* value) {
	config_.set(key, value);
}

static AsrResultType poptype_to_restype(int32_t type) {
	static AsrResultType _tps[] = {
		ASR_RES_ASR,
		ASR_RES_START,
		ASR_RES_END,
	};
	assert(type >= 0 && type < sizeof(_tps)/sizeof(AsrResultType));
	return _tps[type];
}

static AsrError integer_to_reserr(uint32_t err) {
	switch (err) {
		case 0:
			return ASR_SUCCESS;
		case 2:
			return ASR_UNAUTHENTICATED;
		case 3:
			return ASR_CONNECTION_EXCEED;
		case 4:
			return ASR_SERVER_RESOURCE_EXHASTED;
		case 5:
			return ASR_SERVER_BUSY;
		case 6:
			return ASR_SERVER_INTERNAL;
		case 101:
			return ASR_SERVICE_UNAVAILABLE;
		case 102:
			return ASR_SDK_CLOSED;
	}
	return ASR_UNKNOWN;
}

bool AsrImpl::poll(AsrResult& res) {
	shared_ptr<AsrOperationController::Operation> op;
	int32_t id;
	shared_ptr<string> asr;
	int32_t poptype;
	uint32_t err;

	res.asr.clear();

	unique_lock<mutex> locker(resp_mutex_);
	while (initialized_) {
		op = controller_.front_op();
		if (op.get()) {
			if (op->status == AsrStatus::CANCELLED) {
				if (responses_.erase(op->id)) {
					responses_.pop(id, asr, err);
					assert(id == op->id);
				}
				res.id = op->id;
				res.type = ASR_RES_CANCELLED;
				res.err = ASR_SUCCESS;
				controller_.remove_front_op();
				Log::d(tag__, "AsrImpl.poll (%d) cancelled, "
						"remove front op", op->id);
				return true;
			} else if (op->status == AsrStatus::ERROR) {
				if (responses_.erase(op->id)) {
					responses_.pop(id, asr, err);
					assert(id == op->id);
				}
				res.id = op->id;
				res.type = ASR_RES_ERROR;
				res.err = op->error;
				controller_.remove_front_op();
				Log::d(tag__, "AsrImpl.poll (%d) error, "
						"remove front op", op->id);
				return true;
			} else {
				poptype = responses_.pop(id, asr, err);
				if (poptype != StreamQueue<string>::POP_TYPE_EMPTY) {
					assert(id == op->id);
					res.id = id;
					res.type = poptype_to_restype(poptype);
					res.err = integer_to_reserr(err);
					if (res.type == ASR_RES_ASR)
						res.asr = *asr;
					Log::d(tag__, "AsrImpl.poll return result "
							"id(%d), type(%d)", res.id, res.type);
					if (res.type == ASR_RES_END) {
						Log::d(tag__, "AsrImpl.poll (%d) end", res.id);
						controller_.remove_front_op();
					}
					return true;
				}
			}
		}
		Log::d(tag__, "AsrImpl.poll wait");
		resp_cond_.wait(locker);
	}
	Log::d(tag__, "AsrImpl.poll return false, sdk released");
	return false;
}

void AsrImpl::send_reqs() {
	int32_t r;
	int32_t id;
	shared_ptr<string> voice;
	uint32_t err;
	int32_t rv;
	bool opr;

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
			opr = do_ctl_change_op(id, (uint32_t)r);
			locker.unlock();

			if (opr) {
#ifdef SPEECH_SDK_DETAIL_TRACE
				Log::d(tag__, "call do_request %d, %d", id, r);
#endif
				rv = do_request(id, (uint32_t)r, voice);
				if (rv == 0) {
					Log::d(tag__, "AsrImpl.send_reqs wait op finish");
					unique_lock<mutex> resp_locker(resp_mutex_);
					controller_.wait_op_finish(id, resp_locker);
				}
			}
		}
	}
	Log::d(tag__, "thread 'send_reqs' quit");
}

bool AsrImpl::do_ctl_change_op(int32_t id, uint32_t type) {
	shared_ptr<AsrOperationController::Operation> op =
		controller_.current_op();
	if (op.get()) {
		if (type == StreamQueue<string>::POP_TYPE_REMOVED) {
			op->status = AsrStatus::CANCELLED;
			Log::d(tag__, "(%d) is processing, Status --> Cancelled", id);
		}
		return true;
	}
	if (type == StreamQueue<string>::POP_TYPE_START) {
		controller_.new_op(id, AsrStatus::START);
		return true;
	}
	if (type == StreamQueue<string>::POP_TYPE_REMOVED) {
		controller_.new_op(id, AsrStatus::CANCELLED);
		// no data send to server
		// notify 'poll' function to generate 'CANCEL' result
		resp_cond_.notify_one();
		return false;
	}
	Log::d(tag__, "AsrImpl.do_ctl_change_op id is %d, type is %u, "
			"no current op, discard req data", id, type);
	return false;
}

int32_t AsrImpl::do_request(int32_t id, uint32_t type,
		shared_ptr<string>& voice) {
	AsrRequest treq;
	int32_t rv;
	if (type == StreamQueue<string>::POP_TYPE_START) {
		Log::d(tag__, "do_request: send asr start to server. (%d)", id);
		treq.set_id(id);
		treq.set_type(ReqType::START);
		treq.set_lang(config_.get("lang", "zh"));
		treq.set_codec(config_.get("codec", "pcm"));
		treq.set_vt(config_.get("vt", ""));
		rv = 1;
	} else if (type == StreamQueue<string>::POP_TYPE_END
			|| type == StreamQueue<string>::POP_TYPE_REMOVED) {
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

	ConnectionOpResult r = connection_.send(treq, WS_SEND_TIMEOUT);
	if (r != ConnectionOpResult::SUCCESS) {
		AsrError err = ASR_UNKNOWN;
		if (r == ConnectionOpResult::CONNECTION_NOT_AVAILABLE)
			err = ASR_SERVICE_UNAVAILABLE;
		Log::d(tag__, "AsrImpl.do_request: (%d) send failed %d, "
				"set op error", id, r);
		lock_guard<mutex> locker(resp_mutex_);
		controller_.set_op_error(err);
		resp_cond_.notify_one();
		return -1;
	} else if (rv == 0) {
#ifdef SPEECH_SDK_DETAIL_TRACE
		Log::d(tag__, "req (%d) sent, req done", id);
#endif
		lock_guard<mutex> locker(resp_mutex_);
		controller_.refresh_op_time();
	}
	return rv;
}

void AsrImpl::gen_results() {
	AsrResponse resp;
	ConnectionOpResult r;
	AsrError err;
	uint32_t timeout;

	Log::d(tag__, "thread 'gen_results' run");
	while (true) {
		unique_lock<mutex> locker(resp_mutex_);
		timeout = controller_.op_timeout();
		locker.unlock();

#ifdef SPEECH_SDK_DETAIL_TRACE
		Log::d(tag__, "gen_results: recv with timeout %u", timeout);
#endif
		r = connection_.recv(resp, timeout);
		if (r == ConnectionOpResult::NOT_READY)
			break;
		locker.lock();
		if (r == ConnectionOpResult::SUCCESS) {
			gen_result_by_resp(resp);
		} else if (r == ConnectionOpResult::TIMEOUT) {
			if (controller_.op_timeout() == 0) {
				Log::w(tag__, "gen_results: (%d) op timeout, "
						"set op error", controller_.current_op()->id);
				controller_.set_op_error(ASR_TIMEOUT);
				resp_cond_.notify_one();
			}
		} else if (r == ConnectionOpResult::CONNECTION_BROKEN) {
			controller_.set_op_error(ASR_SERVICE_UNAVAILABLE);
			resp_cond_.notify_one();
		} else {
			controller_.set_op_error(ASR_UNKNOWN);
			resp_cond_.notify_one();
		}
		locker.unlock();
	}
	Log::d(tag__, "thread 'gen_results' quit");
}

void AsrImpl::gen_result_by_resp(AsrResponse& resp) {
	bool new_data = false;
	shared_ptr<AsrOperationController::Operation> op;
	op = controller_.current_op();
	if (op.get() && op->id == resp.id()) {
		if (op->status == AsrStatus::START) {
			responses_.start(resp.id());
			op->status = AsrStatus::STREAMING;
			new_data = true;
			Log::d(tag__, "gen_result_by_resp(%d): push start resp, "
					"Status Start --> Streaming", resp.id());
		}

		Log::d(tag__, "AsrResponse has_asr(%d), finish(%d)",
				resp.has_asr(), resp.finish());
		if (resp.has_asr()) {
			shared_ptr<string> asr(new string(resp.asr()));
			responses_.stream(resp.id(), asr);
			new_data = true;
			Log::d(tag__, "gen_result_by_resp(%d): push asr resp "
					"%s", resp.id(), asr->c_str());
		}

		if (resp.finish()) {
			responses_.end(resp.id());
			new_data = true;
			if (op->status != AsrStatus::CANCELLED
					&& op->status != AsrStatus::ERROR) {
				op->status = AsrStatus::END;
				Log::d(tag__, "gen_result_by_resp(%d): push end resp, "
						"Status Streaming --> End", resp.id());
			}
			controller_.finish_op();
		}

		if (new_data) {
			Log::d(tag__, "some responses put to queue, "
					"awake poll thread");
			resp_cond_.notify_one();
		}
	}
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
