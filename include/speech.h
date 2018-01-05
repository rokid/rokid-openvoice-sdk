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
	SPEECH_VAD_TIMEOUT,

	SPEECH_SERVICE_UNAVAILABLE = 101,
	SPEECH_SDK_CLOSED,
	SPEECH_TIMEOUT,
	SPEECH_UNKNOWN,
};

enum SpeechResultType {
	SPEECH_RES_INTER = 0,
	SPEECH_RES_START,
	SPEECH_RES_ASR_FINISH,
	SPEECH_RES_END,
	SPEECH_RES_CANCELLED,
	SPEECH_RES_ERROR
};

typedef struct {
	int32_t id;
	// SPEECH_RES_INTER  intermediate result (part of asr, extra)
	// SPEECH_RES_START  stream result start
	// SPEECH_RES_ASR_FINISH  completly asr result (whole asr)
	// SPEECH_RES_END    completly speech result (whole nlp, action)
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

class SpeechOptions {
public:
	virtual ~SpeechOptions() {}

	// default: Lang::ZH
	virtual void set_lang(Lang lang) = 0;
	// default: Codec::PCM
	virtual void set_codec(Codec codec) = 0;
	// default: VadMode::LOCAL
	virtual void set_vad_mode(VadMode mode, uint32_t timeout = 0) = 0;
	// default: false
	virtual void set_no_nlp(bool value) = 0;
	// default: false
	virtual void set_no_intermediate_asr(bool value) = 0;

	static std::shared_ptr<SpeechOptions> new_instance();
};

class VoiceOptions {
public:
	VoiceOptions();

	VoiceOptions& operator = (const VoiceOptions& options);

	std::string stack;
	std::string voice_trigger;
	uint32_t trigger_start;
	uint32_t trigger_length;
	float voice_power;
	// json string
	// extra data that will send to skill service
	std::string skill_options;
	std::string voice_extra;
};

class Speech {
public:
	virtual ~Speech() {}

	virtual bool prepare(const PrepareOptions& options) = 0;

	virtual void release() = 0;

	virtual int32_t put_text(const char* text, const VoiceOptions* options = NULL) = 0;

	virtual int32_t start_voice(const VoiceOptions* options = NULL) = 0;

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

	virtual void config(const std::shared_ptr<SpeechOptions>& options) = 0;

	static std::shared_ptr<Speech> new_instance();
};


} // namespace speech
} // namespace rokid

