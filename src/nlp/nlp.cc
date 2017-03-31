#include "nlp.h"

using grpc::Status;

namespace rokid {
namespace speech {

Nlp::Nlp() : requests_(NULL), config_(NULL) {
}

Nlp::~Nlp() {
	if (requests_)
		release();
}

bool Nlp::prepare() {
	config_ = new SpeechConfig();
	if (!runner_.prepare(config_)) {
		delete config_;
		config_ = NULL;
		return false;
	}
	callback_manager_.prepare(runner_.responses());
	requests_ = runner_.requests();
	return true;
}

bool Nlp::prepared() {
	return !!requests_;
}

void Nlp::release() {
	if (requests_) {
		requests_ = NULL;
		if (config_) {
			delete config_;
			config_ = NULL;
		}
		callback_manager_.close();
		runner_.close();
	}
}

int Nlp::request(const char* asr, NlpCallback* cb) {
	if (asr == NULL || !requests_)
		return -1;
	int id = callback_manager_.add_callback(cb);
	shared_ptr<string> req(new string(asr));
	requests_->add(id, req);
	return id;
}

void Nlp::cancel(int32_t id) {
	if (requests_ == NULL)
		return;
	if (id <= 0) {
		requests_->clear();
		callback_manager_.clear();
	} else {
		requests_->erase(id);
		callback_manager_.erase_callback(id);
	}
}

void Nlp::config(const char* key, const char* value) {
	// TODO:
}

void NlpCallbackManager::onData(NlpCallback* cb, int id, const NlpRespItem* data) {
	switch (data->type) {
		case 0:
			cb->onNlp(id, data->nlp.c_str());
			break;
		case 1:
			cb->onStop(id);
			break;
		case 2:
			cb->onError(id, data->error);
			break;
		default:
			Log::e("speech.NlpCallback", "onData: invalid callback type %d", data->type);
			assert(false);
	}
}

} // namespace speech
} // namespace rokid
