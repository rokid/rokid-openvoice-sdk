#pragma once

#include "speech_config.h"

namespace rokid {
namespace speech {

typedef struct {
	SpeechConfig config;
} CommonArgument;

typedef struct {
	int32_t id;
	bool deleted;
	std::shared_ptr<std::string> text;
} NlpReqInfo;

#define tag__ "speech.nlp"

} // namespace speech
} // namespace rokid
