#pragma once

#include <map>
#include <string>

using std::map;
using std::string;

namespace rokid {
namespace speech {

class SpeechConfig {
public:
	SpeechConfig();

	bool set(const char* key, const char* value);

	const char* get(const char* key, const char* default_value = NULL) const;

private:
	map<string, string> configs_;
};

} // namespace speech
} // namespace rokid
