#include <stdio.h>
#include <string.h>
#include "speech_config.h"

using std::pair;

namespace rokid {
namespace speech {

SpeechConfig::SpeechConfig() {
}

/**
void SpeechConfig::initialize() {
	configs_.insert(pair<string, string>("server_address", "apigw.open.rokid.com:443"));
	configs_.insert(pair<string, string>("auth_key", "rokid_test_key"));
	configs_.insert(pair<string, string>("device_type", "rokid_test_device_type_id"));
	configs_.insert(pair<string, string>("device_id", "rokid_test_device_id"));
	configs_.insert(pair<string, string>("api_version", "1"));
	configs_.insert(pair<string, string>("secret", "rokid_test_secret"));
}
*/

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

const char* SpeechConfig::get(const char* key, const char* default_value) {
	map<string, string>::iterator it = configs_.find(string(key));
	if (it == configs_.end())
		return default_value;
	return it->second.c_str();
}

} // namespace speech
} // namespace rokid
