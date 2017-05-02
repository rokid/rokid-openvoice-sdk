#pragma once

#include <memory>
#include "speech.pb.h"
#include "speech_config.h"
#include "ws_keepalive.h"
#include "asr.h"

namespace rokid {
namespace speech {

class AsrReqInfo {
public:
	int32_t id;
	uint32_t type;
	std::shared_ptr<std::string> voice;
};

class AsrRespInfo {
public:
	int32_t id;
	AsrError err;
	// 0: wait handle
	// 1: handling
	// 2: handled
	uint32_t status;
};

class AsrCommonArgument {
public:
	SpeechConfig config;

	WSKeepAlive keepalive_;

	inline void start_keepalive(uint32_t interval) {
		keepalive_.start(interval, &config, "asr");
	}
};

#define tag__ "speech.asr"

} // namespace speech
} // namespace rokid
