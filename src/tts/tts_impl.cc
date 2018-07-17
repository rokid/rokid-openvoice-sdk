#include <chrono>
#include "tts_impl.h"
#include "nanopb_encoder.h"

#define WS_SEND_TIMEOUT 5000

namespace rokid {
namespace speech {

using std::shared_ptr;
using std::make_shared;
using std::string;
using std::thread;
using std::mutex;
using std::unique_lock;
using std::lock_guard;
using std::list;
using std::chrono::system_clock;

static const uint32_t MODIFY_CODEC = 1;
static const uint32_t MODIFY_DECLAIMER = 2;
static const uint32_t MODIFY_SAMPLERATE = 4;

class TtsOptionsModifier : public TtsOptionsHolder, public TtsOptions {
public:
	TtsOptionsModifier() {
		TtsOptionsHolder();
		_mask = 0;
	}

	void set_codec(Codec codec) {
		this->codec = codec;
		_mask |= MODIFY_CODEC;
	}

	void set_declaimer(const string& declaimer) {
		this->declaimer = declaimer;
		_mask |= MODIFY_DECLAIMER;
	}

	void modify(TtsOptionsHolder& options) {
		if (_mask & MODIFY_CODEC)
			options.codec = codec;
		if (_mask & MODIFY_DECLAIMER)
			options.declaimer = declaimer;
		if (_mask & MODIFY_SAMPLERATE)
			options.samplerate = samplerate;
	}

	void set_samplerate(uint32_t samplerate) {
		this->samplerate = samplerate;
		_mask |= MODIFY_SAMPLERATE;
	}

private:
	uint32_t _mask;
};

TtsImpl::TtsImpl() : initialized_(false) {
#ifdef SPEECH_STATISTIC
	cur_trace_info_.id = 0;
#endif
}

TtsImpl::~TtsImpl() {
	release();
}

bool TtsImpl::prepare(const PrepareOptions& options) {
	lock_guard<mutex> locker(req_mutex_);
	if (initialized_)
		return true;
	next_id_ = 0;
	connection_.initialize(SOCKET_BUF_SIZE, options, "tts");
	initialized_ = true;
	req_thread_ = new thread([=] { send_reqs(); });
	resp_thread_ = new thread([=] { gen_results(); });
	return true;
}

void TtsImpl::release() {
	KLOGV(tag__, "TtsImpl.release, initialized = %d", initialized_);
	unique_lock<mutex> req_locker(req_mutex_);
	if (initialized_) {
		// notify req thread to exit
		initialized_ = false;
		connection_.release();
		requests_.clear();
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

int32_t TtsImpl::speak(const char* text) {
	if (!initialized_)
		return -1;
	KLOGD(tag__, "speak %s", text);
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

	lock_guard<mutex> locker(req_mutex_);
	if (!initialized_)
		return;
	KLOGV(tag__, "cancel %d", id);
	it = requests_.begin();
	while (it != requests_.end()) {
		if (id <= 0 || (*it)->id == id) {
			(*it)->deleted = true;
			erased = true;
		}
		++it;
	}
	unique_lock<mutex> resp_locker(resp_mutex_);
	if (id <= 0)
		controller_.cancel_op(0, resp_cond_);
	else if (!erased)
		controller_.cancel_op(id, resp_cond_);
	resp_locker.unlock();
}

void TtsImpl::config(const shared_ptr<TtsOptions>& options) {
	if (options.get() == NULL)
		return;
	shared_ptr<TtsOptionsModifier> mod =
		static_pointer_cast<TtsOptionsModifier>(options);
	mod->modify(options_);
}

static TtsResultType poptype_to_restype(int32_t type) {
	static TtsResultType _tps[] = {
		TTS_RES_VOICE,
		TTS_RES_START,
		TTS_RES_END,
	};
	assert(type >= 0 && type < sizeof(_tps)/sizeof(TtsResultType));
	return _tps[type];
}

static TtsError integer_to_reserr(uint32_t err) {
	switch (err) {
		case 0:
			return TTS_SUCCESS;
		case 2:
			return TTS_UNAUTHENTICATED;
		case 3:
			return TTS_CONNECTION_EXCEED;
		case 4:
			return TTS_SERVER_RESOURCE_EXHASTED;
		case 5:
			return TTS_SERVER_BUSY;
		case 6:
			return TTS_SERVER_INTERNAL;
		case 101:
			return TTS_SERVICE_UNAVAILABLE;
		case 102:
			return TTS_SDK_CLOSED;
	}
	return TTS_UNKNOWN;
}

bool TtsImpl::poll(TtsResult& res) {
	shared_ptr<TtsOperationController::Operation> op;
	int32_t id;
	shared_ptr<string> voice;
	int32_t poptype;
	uint32_t err = 0;

	res.voice.reset();
	res.err = TTS_SUCCESS;

	unique_lock<mutex> locker(resp_mutex_);
	while (initialized_) {
		op = controller_.front_op();
		if (op.get()) {
			if (op->status == TtsStatus::CANCELLED) {
				if (responses_.erase(op->id)) {
					responses_.pop(id, voice, err);
					assert(id == op->id);
				}
				res.id = op->id;
				res.type = TTS_RES_CANCELLED;
				res.err = TTS_SUCCESS;
				controller_.remove_front_op();
				KLOGV(tag__, "TtsImpl.poll (%d) cancelled, "
						"remove front op", op->id);
				return true;
			} else if (op->status == TtsStatus::ERROR) {
				if (responses_.erase(op->id)) {
					responses_.pop(id, voice, err);
					assert(id == op->id);
				}
				res.id = op->id;
				res.type = TTS_RES_ERROR;
				res.err = op->error;
				controller_.remove_front_op();
				KLOGV(tag__, "TtsImpl.poll (%d) error, "
						"remove front op", op->id);
				return true;
			} else {
				poptype = responses_.pop(id, voice, err);
				if (poptype != TtsStreamQueue::POP_TYPE_EMPTY) {
					assert(id == op->id);
					res.id = id;
					res.type = poptype_to_restype(poptype);
					res.err = integer_to_reserr(err);
					res.voice = voice;
					KLOGV(tag__, "TtsImpl.poll return result id(%d), "
							"type(%d)", res.id, res.type);
					if (res.type == TTS_RES_END) {
						KLOGV(tag__, "TtsImpl.poll (%d) end", res.id);
						controller_.remove_front_op();
					}
					return true;
				}
			}
		}
		KLOGV(tag__, "TtsImpl.poll wait");
		resp_cond_.wait(locker);
	}
	KLOGV(tag__, "TtsImpl.poll return false, sdk released");
	return false;
}

void TtsImpl::send_reqs() {
	shared_ptr<TtsReqInfo> req;
	TtsStatus status;
	KLOGV(tag__, "thread 'send_reqs' begin");
	while (true) {
		unique_lock<mutex> locker(req_mutex_);
		if (!initialized_)
			break;
		if (requests_.empty()) {
			KLOGV(tag__, "TtsImpl.send_reqs wait req available");
			req_cond_.wait(locker);
		} else {
			req = requests_.front();
			requests_.pop_front();
			status = do_ctl_new_op(req);
			locker.unlock();

			if (status == TtsStatus::START && do_request(req)) {
				KLOGV(tag__, "TtsImpl.send_reqs wait op finish");
				unique_lock<mutex> resp_locker(resp_mutex_);
				controller_.wait_op_finish(req->id, resp_locker);
			}
		}
	}
	KLOGV(tag__, "thread 'send_reqs' quit");
}

TtsStatus TtsImpl::do_ctl_new_op(shared_ptr<TtsReqInfo>& req) {
	lock_guard<mutex> locker(resp_mutex_);
	if (req->deleted) {
		KLOGV(tag__, "do_ctl_new_op: cancelled");
		controller_.new_op(req->id, TtsStatus::CANCELLED);
		resp_cond_.notify_one();
		return TtsStatus::CANCELLED;
	}
	KLOGV(tag__, "do_ctl_new_op: start");
	controller_.new_op(req->id, TtsStatus::START);
	return TtsStatus::START;
}

static const char* get_codec_str(Codec codec) {
	switch (codec) {
	case Codec::PCM:
		return "pcm";
	case Codec::OPU:
		return "opu";
	case Codec::OPU2:
		return "opu2";
	case Codec::MP3:
		return "mp3";
	}
	return NULL;
}

bool TtsImpl::do_request(shared_ptr<TtsReqInfo>& req) {
	KLOGV(tag__, "do_request: send req to server. (%d:%s), codec(%s), declaimer(%s), samplerate(%u)",
			req->id, req->data.c_str(), get_codec_str(options_.codec),
			options_.declaimer.c_str(), options_.samplerate);
	TtsRequest treq;
	treq.set_id(req->id);
	treq.set_text(req->data.c_str());
	treq.set_declaimer(options_.declaimer);
	treq.set_codec(get_codec_str(options_.codec));
	treq.set_sample_rate(options_.samplerate);
#ifdef SPEECH_STATISTIC
	cur_trace_info_.id = req->id;
	cur_trace_info_.req_tp = system_clock::now();
#endif
	ConnectionOpResult r = connection_.send(treq, WS_SEND_TIMEOUT);
	if (r != ConnectionOpResult::SUCCESS) {
		TtsError err = TTS_UNKNOWN;
		if (r == ConnectionOpResult::CONNECTION_NOT_AVAILABLE)
			err = TTS_SERVICE_UNAVAILABLE;
		KLOGW(tag__, "do_request: (%d) send req failed %d, "
				"set op error", req->id, r);
		lock_guard<mutex> locker(resp_mutex_);
		controller_.set_op_error(err);
		resp_cond_.notify_one();
		return false;
	}
	KLOGD(tag__, "req (%d) sent, req done", req->id);
	lock_guard<mutex> locker(resp_mutex_);
	controller_.refresh_op_time(false);
	return true;
}

void TtsImpl::gen_results() {
	TtsResponse resp;
	ConnectionOpResult r;
	TtsError err;
	uint32_t timeout;

	KLOGV(tag__, "thread 'gen_results' run");
	while (true) {
		unique_lock<mutex> locker(resp_mutex_);
		timeout = controller_.op_timeout();
		locker.unlock();

		r = connection_.recv(resp, timeout);
		if (r == ConnectionOpResult::NOT_READY)
			break;
		locker.lock();
		if (r == ConnectionOpResult::SUCCESS) {
			controller_.refresh_op_time(true);
			gen_result_by_resp(resp);
		} else if (r == ConnectionOpResult::TIMEOUT) {
			if (controller_.op_timeout() == 0) {
				KLOGI(tag__, "gen_results: (%d) op timeout, "
						"set op error", controller_.current_op()->id);
				controller_.set_op_error(TTS_TIMEOUT);
				resp_cond_.notify_one();
#ifdef SPEECH_STATISTIC
				finish_cur_req();
#endif
			}
		} else if (r == ConnectionOpResult::CONNECTION_BROKEN) {
			controller_.set_op_error(TTS_SERVICE_UNAVAILABLE);
			resp_cond_.notify_one();
#ifdef SPEECH_STATISTIC
			finish_cur_req();
#endif
		} else {
			controller_.set_op_error(TTS_UNKNOWN);
			resp_cond_.notify_one();
#ifdef SPEECH_STATISTIC
			finish_cur_req();
#endif
		}
		locker.unlock();
	}
	KLOGV(tag__, "thread 'gen_results' quit");
}

void TtsImpl::gen_result_by_resp(TtsResponse& resp) {
	bool new_data = false;
	shared_ptr<TtsOperationController::Operation> op;
	op = controller_.current_op();
	if (op.get() && op->id == resp.id()) {
		if (op->status == TtsStatus::START) {
			responses_.start(resp.id());
			new_data = true;
			op->status = TtsStatus::STREAMING;
			KLOGD(tag__, "gen_result_by_resp(%d): push start resp, "
					"Status Start --> Streaming", resp.id());
		}

		KLOGV(tag__, "TtsResponse has_voice(%d), finish(%d)",
				resp.has_voice(), resp.finish());
		if (resp.has_voice()) {
			shared_ptr<string> voice;
#ifdef LOW_PB_VERSION
			voice.reset(new string(resp.voice()));
#else
			voice.reset(resp.release_voice());
#endif
			responses_.stream(resp.id(), voice);
			new_data = true;
			KLOGV(tag__, "gen_result_by_resp(%d): push voice "
					"resp, %d bytes", resp.id(), voice->length());
		}

		if (resp.finish()) {
			responses_.end(resp.id());
			new_data = true;
			if (op->status != TtsStatus::CANCELLED
					&& op->status != TtsStatus::ERROR) {
				op->status = TtsStatus::END;
				KLOGD(tag__, "gen_result_by_resp(%d): push end resp, "
						"Status Streaming --> End", resp.id());
			}
			controller_.finish_op();
#ifdef SPEECH_STATISTIC
			finish_cur_req();
#endif
		}

		if (new_data) {
			KLOGV(tag__, "some responses put to queue, "
					"awake poll thread");
			resp_cond_.notify_one();
		}
	}
}

#ifdef SPEECH_STATISTIC
void TtsImpl::finish_cur_req() {
	if (cur_trace_info_.id) {
		cur_trace_info_.resp_tp = system_clock::now();
		connection_.add_trace_info(cur_trace_info_);
		cur_trace_info_.id = 0;
	}
}
#endif

shared_ptr<Tts> Tts::new_instance() {
	return make_shared<TtsImpl>();
}

TtsOptionsHolder::TtsOptionsHolder() : codec(Codec::PCM), declaimer("zh"), samplerate(24000) {
}

shared_ptr<TtsOptions> TtsOptions::new_instance() {
	return make_shared<TtsOptionsModifier>();
}

} // namespace speech
} // namespace rokid
