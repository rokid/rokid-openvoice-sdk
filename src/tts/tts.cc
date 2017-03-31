#include "tts.h"
#include "speech_config.h"
#include "log.h"

namespace rokid {
namespace speech {

const char* Tts::tag_ = "speech.tts";

Tts::Tts() : requests_(NULL) {
	config_ = new SpeechConfig();
}

Tts::~Tts() {
	if (requests_)
		release();
	if (config_) {
		delete config_;
		config_ = NULL;
	}
}

bool Tts::prepare(const char* config_file) {
	if (!runner_.prepare(config_))
		return false;
	requests_ = runner_.requests();
	callback_manager_.prepare(runner_.responses());
	return true;
}

void Tts::release() {
	if (requests_) {
		runner_.close();
		callback_manager_.close();
		requests_ = NULL;
	}
}

bool Tts::prepared() {
	return !!requests_;
}

int32_t Tts::speak(const char* content, TtsCallback* cb) {
	if (requests_ == NULL || content == NULL)
		return -1;
	int32_t id = callback_manager_.add_callback(cb);
	shared_ptr<string> c(new string(content));
	if (!requests_->add(id, c)) {
		callback_manager_.erase_callback(id);
		return -1;
	}
	Log::d(tag_, "speak %s, id %d", content, id);
	return id;
}

void Tts::stop(int32_t id) {
	Log::d(tag_, "stop id %d", id);
	if (id == 0) {
		runner_.clear();
		callback_manager_.clear();
	} else if (id > 0) {
		runner_.cancel(id);
		callback_manager_.erase_callback(id);
	}
}

void Tts::config(const char* key, const char* value) {
	if (config_)
		config_->set(key, value);
}

bool Tts::config_valid(const char* key, const char* value) {
	return key && strlen(key) > 0;
}

// debug
/**
void printHexData(uint8_t* data, uint32_t len) {
	int i = 0;
	char* buf = new char[len * 2 + 1];
	char* p;
	while (i < len) {
		sprintf(p, "%x", data[i]);
		p += 2;
		++i;
	}
	buf[len * 2] = '\x0';
	Log::d("speech.debug", "%s", buf);
	delete buf;
}
*/
// debug

void TtsCallbackManager::onData(TtsCallback* cb, int32_t id, const TtsResponse* data) {
	string str;

	str = data->text();
	cb->onText(id, str.c_str());
	str = data->voice();
	// Log::d("speech.debug", "==============================================");
	// printHexData((uint8_t*)str.c_str(), str.length());
	// Log::d("speech.debug", "==============================================");
	// printHexData((uint8_t*)str.data(), str.length());
	Log::d(Tts::tag_, "callback onVoice length %d", str.length());
	cb->onVoice(id, (const uint8_t*)str.data(), str.length());
}

} // namespace speech
} // namespace rokid
