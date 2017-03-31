#include "asr.h"

namespace rokid {
namespace speech {

Asr::Asr() : requests_(NULL), config_(NULL) {
}

Asr::~Asr() {
	if (requests_)
		release();
}

bool Asr::prepare() {
	config_ = new SpeechConfig();
	if (!runner_.prepare(config_)) {
		delete config_;
		config_ = NULL;
		return false;
	}
	requests_ = runner_.requests();
	callback_manager_.prepare(runner_.responses());
	return true;
}

bool Asr::prepared() {
	return !!requests_;
}

void Asr::release() {
	if (requests_) {
		runner_.close();
		callback_manager_.close();
		requests_ = NULL;
		if (config_) {
			delete config_;
			config_ = NULL;
		}
	}
}

int Asr::start(AsrCallback* cb) {
	if (!requests_)
		return -1;
	int id = callback_manager_.add_callback(cb);
	if (!requests_->start(id)) {
		callback_manager_.erase_callback(id);
		return -1;
	}
	return id;
}

void Asr::voice(int id, const unsigned char* data, int length) {
	if (!requests_ || data == NULL || length == 0)
		return;
	shared_ptr<string> sp(new string((const char*)data, length));
	requests_->stream(id, sp);
}

void Asr::end(int id) {
	if (!requests_)
		return;
	requests_->end(id);
}

void Asr::cancel(int id) {
	if (!requests_)
		return;
	if (id <= 0) {
		runner_.clear();
		callback_manager_.clear();
	} else {
		runner_.cancel(id);
		callback_manager_.erase_callback(id);
	}
}

void Asr::config(const char* key, const char* value) {
	if (config_)
		config_->set(key, value);
}

const char* AsrCallbackManager::tag_ = "AsrCallbackManager";
void AsrCallbackManager::onData(AsrCallback* cb, int id, const AsrResponse* data) {
	Log::d(tag_, "onData: %d:%s", id, data->asr().c_str());
	cb->onData(id, data->asr().c_str());
}

} // namespace speech
} // namespace rokid
