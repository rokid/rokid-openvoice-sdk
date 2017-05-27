#pragma once

#include <string>

namespace rokid {
namespace speech {

#define SOCKET_BUF_SIZE 0x40000

typedef struct {
	int32_t id;
	bool deleted;
	std::string data;
} TtsReqInfo;

/**
typedef struct {
	int32_t id;
	TtsError err;
} TtsRespInfo;
*/

enum TtsStatus {
	TTS_STATUS_START = 0,
	TTS_STATUS_STREAMING,
	TTS_STATUS_END,
	TTS_STATUS_CANCELLED,
	TTS_STATUS_ERROR
};

/**
class TtsCommonArgument {
public:
	int32_t current_id;
	SpeechConfig config;
	SpeechConnection connection;

	inline void init() {
		connection.initialize(SOCKET_BUF_SIZE, &config, "tts");
	}

	inline void destroy() {
		connection.release();
	}
};
*/

#define tag__ "speech.tts"

} // namespace speech
} // namespace rokid
