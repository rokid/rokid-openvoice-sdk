#pragma once

#include <stdint.h>
#include <string>

namespace rokid {
namespace speech {

struct SpeechResult {
	int32_t id;
	uint32_t err;
	std::string asr;
	std::string nlp;
	std::string action;
};

class Speech {
public:
	virtual ~Speech() {}

	virtual bool prepare(const char* config_file = NULL) = 0;

	virtual void release() = 0;

	virtual int32_t put_text(const char* text) = 0;

	virtual int32_t start_voice() = 0;

	virtual void put_voice(int32_t id, const uint8_t* data, uint32_t length) = 0;

	virtual void end_voice(int32_t id) = 0;

	virtual void cancel(int32_t id) = 0;

	// poll speech results
	// block current thread if no result available
	// if Speech.release() invoked, poll() will return -1
	//
	// return value: 0  speech result
	//               1  stream result start
	//               2  stream result end
	//               3  speech cancelled
	//               4  speech occurs error, see SpeechResult.err
	//               -1  speech sdk released
	virtual int32_t poll(SpeechResult& res) = 0;

	virtual void config(const char* key, const char* value) = 0;
};

Speech* new_speech();

void delete_speech(Speech* speech);

} // namespace speech
} // namespace rokid

