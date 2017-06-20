#include <string.h>
#include <chrono>
#include "speech_config.h"

using std::pair;
using std::map;
using std::string;
using std::mutex;
using std::lock_guard;
using std::shared_ptr;

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

static void set_json_property(string& json, const string& key,
		const string& value, bool comma) {
	if (comma)
		json.append(",");
	json.append("\"");
	json.append(key);
	json.append("\":\"");
	json.append(value);
	json.append("\"");
}

void SpeechConfig::to_json_string(std::string& json) {
	map<string, string>::const_iterator it;

	json.clear();
	json.append("{");
	for (it = configs_.begin(); it != configs_.end(); ++it) {
		set_json_property(json, it->first, it->second, it != configs_.begin());
	}
	json.append("}");
}

shared_ptr<Options> new_options() {
	return shared_ptr<Options>(new SpeechConfig());
}

} // namespace speech
} // namespace rokid
