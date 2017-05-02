#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include "speech.pb.h"
#include "speech_config.h"
#include "speech.h"
#include "ws_keepalive.h"

namespace rokid {
namespace speech {

typedef struct {
	int32_t id;
	// 0  text req
	// 1  voice req start
	// 2  voice req end
	// 3  voice req cancel
	// 4  voice req data
	uint32_t type;
	std::shared_ptr<std::string> data;
} SpeechReqInfo;

typedef struct {
	int32_t id;
	SpeechError err;
	// 0: wait handle
	// 1: handling
	// 2: handled
	uint32_t status;
} SpeechRespInfo;

class SpeechCommonArgument {
public:
	SpeechConfig config;

	WSKeepAlive keepalive_;

	inline void start_keepalive(uint32_t interval) {
		keepalive_.start(interval, &config, "speech");
	}
};

#define tag__ "speech.speech"

} // namespace speech
} // namespace rokid
