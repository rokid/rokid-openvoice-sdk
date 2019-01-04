#pragma once

#include <string>

namespace rokid {
namespace speech {

#define SOCKET_BUF_SIZE 0x40000

typedef struct {
	std::shared_ptr<std::string> voice;
	std::shared_ptr<std::string> text;
} TtsResultIn;

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

enum class TtsStatus {
	START = 0,
	STREAMING,
	END,
	CANCELLED,
	ERROR
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
