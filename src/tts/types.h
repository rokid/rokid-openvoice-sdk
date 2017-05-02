#pragma once

#include "speech.pb.h"
#include "speech_config.h"
#include "ws_keepalive.h"
#include "tts.h"

namespace rokid {
namespace speech {

typedef struct {
	int32_t id;
	bool deleted;
	std::shared_ptr<std::string> data;
} TtsReqInfo;

typedef struct {
	int32_t id;
	TtsError err;
} TtsRespInfo;

class TtsCommonArgument {
public:
	int32_t current_id;
	SpeechConfig config;
	WSKeepAlive keepalive_;

	inline void start_keepalive(uint32_t interval) {
		keepalive_.start(interval, &config, "tts");
	}
};

#define tag__ "speech.tts"

} // namespace speech
} // namespace rokid
