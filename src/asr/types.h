#pragma once

#include <memory>
#include "speech.pb.h"
#include "speech_config.h"
#include "speech_connection.h"
#include "asr.h"

#define SOCKET_BUF_SIZE 0x40000

namespace rokid {
namespace speech {

class AsrReqInfo {
public:
	int32_t id;
	uint32_t type;
	std::shared_ptr<std::string> voice;
};

/**
class AsrRespInfo {
public:
	int32_t id;
	AsrError err;
	// 0: wait handle
	// 1: handling
	// 2: handled
	uint32_t status;
};
*/

enum class AsrStatus {
	START = 0,
	STREAMING,
	END,
	CANCELLED,
	ERROR
};

/**
class AsrCommonArgument {
public:
	SpeechConfig config;

	SpeechConnection connection;

	inline void init() {
		connection.initialize(SOCKET_BUF_SIZE, &config, "asr");
	}

	inline void destroy() {
		connection.release();
	}
};
*/

#define tag__ "speech.asr"

} // namespace speech
} // namespace rokid
