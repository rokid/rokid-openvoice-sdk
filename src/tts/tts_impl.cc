#include "tts_impl.h"
#include "speech_connection.h"

#define PING_INTERVAL_TIME 20000

namespace rokid {
namespace speech {

TtsImpl::TtsImpl() : Pipeline(tag__)
                   , next_id_(0)
                   , requests_(req_provider_.queue()) {
}

bool TtsImpl::prepare() {
	requests_->reset();
	cancel_handler_.reset();
	pipeline_arg_.start_keepalive(PING_INTERVAL_TIME);
	req_handler_.set_cancel_handler(&cancel_handler_);
	set_head(&req_provider_);
	add(&req_handler_, &pipeline_arg_);
	add(&resp_handler_, &pipeline_arg_);
	add(&cancel_handler_, NULL);
	// worker for req handler + resp handler + cancel handler
	add_worker(0);
	run();
	return true;
}

void TtsImpl::release() {
	if (!closed()) {
		requests_->close();
		cancel_handler_.close();
		close();
		pipeline_arg_.keepalive_.close();
	}
}

int32_t TtsImpl::speak(const char* text) {
	int32_t id = next_id();
	shared_ptr<string> content(new string(text));
	if (!requests_->add(id, content))
		return -1;
	return id;
}

void TtsImpl::cancel(int32_t id) {
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

void TtsImpl::config(const char* key, const char* value) {
	pipeline_arg_.config.set(key, value);
}

bool TtsImpl::poll(TtsResult& res) {
	shared_ptr<TtsResult> t = cancel_handler_.poll();
	if (t.get() == NULL)
		return false;
	res = *t;
	return true;
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
