#include <chrono>
#include "speech_impl.h"
#include "speech.grpc.pb.h"
#include "speech_connection.h"

using std::shared_ptr;
using std::mutex;
using std::lock_guard;

namespace rokid {
namespace speech {

SpeechImpl::SpeechImpl() : Pipeline(tag__), next_id_(0) {
}

bool SpeechImpl::prepare() {
	req_handler_.set_cancel_handler(&cancel_handler_);
	req_provider_.set_cancel_handler(&cancel_handler_);
	set_head(&req_provider_);
	add(&req_handler_, &pipeline_arg_);
	add(&resp_handler_, &pipeline_arg_);
	add(&cancel_handler_, NULL);
	// worker for req handler
	add_worker(1);
	// worker for resp handler
	add_worker(1);
	// worker for cancel handler
	add_worker(0);
	run();
	return true;
}

void SpeechImpl::release() {
	if (!closed()) {
		req_provider_.close();
		req_handler_.close();
		resp_handler_.close();
		cancel_handler_.close();
		close();
		// at last, close grpc connection
		pipeline_arg_.reset_stub();
	}
}

int32_t SpeechImpl::put_text(const char* text) {
	if (text == NULL)
		return -1;
	int32_t id = next_id();
	req_provider_.put_text(id, text);
	return id;
}

int32_t SpeechImpl::start_voice() {
	int32_t id = next_id();
	req_provider_.start_voice(id);
	return id;
}

void SpeechImpl::put_voice(int32_t id, const uint8_t* data, uint32_t length) {
	if (data == NULL || length == 0)
		return;
	req_provider_.put_voice(id, data, length);
}

void SpeechImpl::end_voice(int32_t id) {
	req_provider_.end_voice(id);
}

void SpeechImpl::cancel(int32_t id) {
	req_provider_.cancel(id);
}

void SpeechImpl::config(const char* key, const char* value) {
	pipeline_arg_.config.set(key, value);
}

bool SpeechImpl::poll(SpeechResult& res) {
	shared_ptr<SpeechResult> r = cancel_handler_.poll();
	if (r.get() == NULL)
		return false;
	memcpy(&res, r.get(), sizeof(res));
	return true;
}

Speech* new_speech() {
	return new SpeechImpl();
}

void delete_speech(Speech* speech) {
	if (speech) {
		delete speech;
	}
}

shared_ptr<rokid::open::Speech::Stub> CommonArgument::stub() {
	lock_guard<mutex> locker(mutex_);
	if (stub_.get() == NULL)
		stub_ = SpeechConnection::connect(&config, "speech");
	return stub_;
}

void CommonArgument::reset_stub() {
	lock_guard<mutex> locker(mutex_);
	stub_.reset();
}

} // namespace speech
} // namespace rokid
