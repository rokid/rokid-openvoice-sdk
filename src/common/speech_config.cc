#include <stdio.h>
#include <string.h>
#include "speech_config.h"

using std::pair;

namespace rokid {
namespace speech {

SpeechConfig::SpeechConfig() {
}

bool SpeechConfig::set(const char* key, const char* value) {
	if (key == NULL)
		return false;
	string skey(key);
	if (value == NULL || strlen(value) == 0) {
		// delete key
		configs_.erase(skey);
	} else {
		// set key value
		configs_[skey] = string(value);
	}
	return true;
}

const char* SpeechConfig::get(const char* key, const char* default_value) const {
	map<string, string>::const_iterator it = configs_.find(string(key));
	if (it == configs_.end())
		return default_value;
	return it->second.c_str();
}

} // namespace speech
} // namespace rokid
