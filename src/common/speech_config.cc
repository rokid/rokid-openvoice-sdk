#include <string.h>
#include <chrono>
#include "speech_config.h"

using std::pair;
using std::map;
using std::string;
using std::mutex;
using std::lock_guard;

namespace rokid {
namespace speech {

bool SpeechConfig::set(const char* key, const char* value) {
	if (key == NULL)
		return false;
	string skey(key);
	lock_guard<mutex> locker(mutex_);
	if (value == NULL || strlen(value) == 0) {
		// delete key
		configs_.erase(skey);
	} else {
		// set key value
		configs_[skey] = string(value);
	}
	return true;
}

const char* SpeechConfig::get(const char* key, const char* default_value) {
	lock_guard<mutex> locker(mutex_);
	map<string, string>::const_iterator it = configs_.find(string(key));
	if (it == configs_.end())
		return default_value;
	return it->second.c_str();
}

void SpeechConfig::clear() {
	lock_guard<mutex> locker(mutex_);
	configs_.clear();
}

} // namespace speech
} // namespace rokid
