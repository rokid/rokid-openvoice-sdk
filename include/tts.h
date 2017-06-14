#pragma once

#include <stdint.h>
#include <string>
#include <memory>

namespace rokid {
namespace speech {

enum TtsError {
	// server error
	TTS_SUCCESS = 0,
	TTS_UNAUTHENTICATED = 2,
	TTS_CONNECTION_EXCEED,
	TTS_SERVER_RESOURCE_EXHASTED,
	TTS_SERVER_BUSY,
	TTS_SERVER_INTERNAL,

	// client error
	TTS_SERVICE_UNAVAILABLE = 101,
	TTS_SDK_CLOSED,
	TTS_TIMEOUT,
	TTS_UNKNOWN,
};

enum TtsResultType {
	TTS_RES_VOICE = 0,
	TTS_RES_START,
	TTS_RES_END,
	TTS_RES_CANCELLED,
	TTS_RES_ERROR
};

struct TtsResult {
	// 0  tts result
	// 1  stream result start
	// 2  stream result end
	// 3  tts cancelled
	// 4  tts occurs error, see TtsResult.err
	TtsResultType type;
	int32_t id;
	TtsError err;
	std::shared_ptr<std::string> voice;
};

class Tts {
public:
	virtual ~Tts() {}

	virtual bool prepare() = 0;

	virtual void release() = 0;

	virtual int32_t speak(const char* text) = 0;

	// param id:  > 0  cancel tts request specified by 'id'
	//           <= 0  cancel all tts requests
	virtual void cancel(int32_t id) = 0;

	// poll tts results
	// block current thread if no result available
	// if Tts.release() invoked, poll() will return -1
	//
	// return value  true  success
	//               false tts sdk released
	virtual bool poll(TtsResult& res) = 0;

	// key:  'server_address'  value:  default is 'apigw.open.rokid.com:443',
	//       'ssl_roots_pem'           your roots.pem file path
	//       'auth_key'                'your_auth_key'
	//       'device_type_id'          'your_device_type_id'
	//       'device_id'               'your_device_id'
	//       'api_version'             now is '1'
	//       'secret'                  'your_secret'
	//       'codec'                   'pcm' (default)
	//                                 'opu'
	//                                 'opu2'
	//       'declaimer'               'zh' (default)
	virtual void config(const char* key, const char* value) = 0;
};

Tts* new_tts();

void delete_tts(Tts* tts);

} // namespace speech
} // namespace rokid

