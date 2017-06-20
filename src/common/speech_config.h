#pragma once

#include <map>
#include <string>
#include <mutex>
#include "speech_types.h"

namespace rokid {
namespace speech {

class SpeechConfig : public Options {
public:
	bool set(const char* key, const char* value);

	const char* get(const char* key, const char* default_value = NULL);

	void clear();

	void to_json_string(std::string& json);

private:
	std::map<std::string, std::string> configs_;
	std::mutex mutex_;
};

} // namespace speech
} // namespace rokid
