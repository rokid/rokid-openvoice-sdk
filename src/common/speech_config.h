#pragma once

#include <map>
#include <string>
#include <mutex>

namespace rokid {
namespace speech {

class SpeechConfig {
public:
	bool set(const char* key, const char* value);

	const char* get(const char* key, const char* default_value = NULL);

	void clear();

private:
	std::map<std::string, std::string> configs_;
	std::mutex mutex_;
};

} // namespace speech
} // namespace rokid
