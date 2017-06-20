#pragma once

#include <stdint.h>
#include <string>
#include "speech_types.h"

namespace rokid {
namespace speech {

enum SpeechError {
	SPEECH_SUCCESS = 0,
	SPEECH_UNAUTHENTICATED = 2,
	SPEECH_CONNECTION_EXCEED,
	SPEECH_SERVER_RESOURCE_EXHASTED,
	SPEECH_SERVER_BUSY,
	SPEECH_SERVER_INTERNAL,

	SPEECH_SERVICE_UNAVAILABLE = 101,
	SPEECH_SDK_CLOSED,
	SPEECH_TIMEOUT,
	SPEECH_UNKNOWN,
};

enum SpeechResultType {
	SPEECH_RES_INTER = 0,
	SPEECH_RES_START,
	SPEECH_RES_END,
	SPEECH_RES_CANCELLED,
	SPEECH_RES_ERROR
};

typedef struct {
	int32_t id;
	// SPEECH_RES_INTER  intermediate result (part of asr, extra)
	// SPEECH_RES_START  stream result start
	// SPEECH_RES_END    completly speech result (whole asr, nlp, action)
	// SPEECH_RES_CANCELLED  speech cancelled
	// SPEECH_RES_ERROR  speech occurs error
	SpeechResultType type;
	SpeechError err;
	std::string asr;
	std::string nlp;
	std::string action;
	// json string
	// {
	//   "activation": "fake|reject|accept|none"
	// }
	std::string extra;
} SpeechResult;

class Speech {
public:
	virtual ~Speech() {}

	virtual bool prepare() = 0;

	virtual void release() = 0;

	virtual int32_t put_text(const char* text) = 0;

	virtual int32_t start_voice(
			std::shared_ptr<Options> framework_options = std::shared_ptr<Options>(),
			std::shared_ptr<Options> skill_options = std::shared_ptr<Options>()
			) = 0;

	virtual void put_voice(int32_t id, const uint8_t* data, uint32_t length) = 0;

	virtual void end_voice(int32_t id) = 0;

	virtual void cancel(int32_t id) = 0;

	// poll speech results
	// block current thread if no result available
	// if Speech.release() invoked, poll() will return false
	//
	// return value: true  success
	//               false speech sdk released
	virtual bool poll(SpeechResult& res) = 0;

	virtual void config(const char* key, const char* value) = 0;
};

std::shared_ptr<Speech> new_speech();

} // namespace speech
} // namespace rokid

