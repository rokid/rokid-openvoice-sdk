#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include "speech.pb.h"
#include "speech_config.h"
#include "speech.h"

#define SOCKET_BUF_SIZE 0x40000

namespace rokid {
namespace speech {

enum class SpeechReqType {
	TEXT,
	VOICE_START,
	VOICE_END,
	CANCELLED,
	VOICE_DATA,
};

typedef struct {
	int32_t id;
	SpeechReqType type;
	std::shared_ptr<std::string> data;
} SpeechReqInfo;

/**
typedef struct {
	int32_t id;
	SpeechError err;
	// 0: wait handle
	// 1: handling
	// 2: handled
	uint32_t status;
} SpeechRespInfo;
*/

enum class SpeechStatus {
	START = 0,
	STREAMING,
	END,
	CANCELLED,
	ERROR
};

typedef struct {
	std::string asr;
	std::string nlp;
	std::string action;
} SpeechResultIn;

/**
class SpeechCommonArgument {
public:
	SpeechConfig config;

	WSKeepAlive keepalive_;

	inline void start_keepalive(uint32_t interval) {
		keepalive_.start(interval, &config, "speech");
	}
};
*/

#define tag__ "speech.speech"

} // namespace speech
} // namespace rokid
