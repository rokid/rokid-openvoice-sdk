#include "asr_impl.h"
#include "speech_connection.h"
#include "speech.grpc.pb.h"
#include "log.h"

using std::unique_ptr;
using std::shared_ptr;
using std::string;
using rokid::open::Speech;

namespace rokid {
namespace speech {

AsrImpl::AsrImpl() : Pipeline(tag__), requests_(req_provider_.queue()), next_id_(0) {
}

bool AsrImpl::prepare() {
	req_handler_.set_cancel_handler(&cancel_handler_);
	set_head(&req_provider_);
	add(&req_handler_, &pipeline_arg_);
	add(&resp_handler_, &pipeline_arg_);
	add(&cancel_handler_, NULL);
	// worker for req handler
	add_worker(1);
	// worker for resp handler + cancel handler
	add_worker(0);
	run();
	return true;
}

void AsrImpl::release() {
	if (!closed()) {
		requests_->close();
		req_handler_.close();
		cancel_handler_.close();
		close();
		// at last, close grpc connection
		pipeline_arg_.reset_stub();
	}
}

int32_t AsrImpl::start() {
	int32_t id = next_id();
	if (!requests_->start(id))
		return -1;
	Log::d(tag__, "AsrImpl: start %d", id);
	return id;
}

void AsrImpl::voice(int32_t id, const uint8_t* voice, uint32_t length) {
	if (id <= 0)
		return;
	if (voice == NULL || length == 0)
		return;
	Log::d(tag__, "AsrImpl: voice %d, len %u", id, length);
	shared_ptr<string> spv(new string((const char*)voice, length));
	requests_->stream(id, spv);
}

void AsrImpl::end(int32_t id) {
	if (id <= 0)
		return;
	Log::d(tag__, "AsrImpl: end %d", id);
	requests_->end(id);
}

void AsrImpl::cancel(int32_t id) {
	Log::d(tag__, "AsrImpl: cancel %d", id);
	if (id > 0) {
		if (!requests_->erase(id))
			cancel_handler_.cancel(id);
	} else {
		int32_t min_id;
		requests_->clear(&min_id, NULL);
		// try cancel previous polled request
		if (min_id > 1)
			cancel_handler_.cancel(min_id - 1);
	}
}

void AsrImpl::config(const char* key, const char* value) {
	pipeline_arg_.config.set(key, value);
}

bool AsrImpl::poll(AsrResult& res) {
	shared_ptr<AsrResult> tmp = cancel_handler_.poll();
	if (tmp.get() == NULL)
		return false;
	res = *tmp;
	return true;
}

Asr* new_asr() {
	return new AsrImpl();
}

void delete_asr(Asr* asr) {
	if (asr)
		delete asr;
}

shared_ptr<rokid::open::Speech::Stub> CommonArgument::stub() {
	lock_guard<mutex> locker(mutex_);
	if (stub_.get() == NULL)
		stub_ = SpeechConnection::connect(&config, "asr");
	return stub_;
}

void CommonArgument::reset_stub() {
	lock_guard<mutex> locker(mutex_);
	stub_.reset();
}

} // namespace speech
} // namespace rokid
