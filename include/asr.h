#pragma once

#include <stdint.h>
#include <string>
#include <memory>

namespace rokid {
namespace speech {

enum AsrError {
	ASR_SUCCESS = 0,
	ASR_UNAUTHENTICATED = 2,
	ASR_CONNECTION_EXCEED,
	ASR_SERVER_RESOURCE_EXHASTED,
	ASR_SERVER_BUSY,
	ASR_SERVER_INTERNAL,

	ASR_SERVICE_UNAVAILABLE = 101,
	ASR_SDK_CLOSED,
	ASR_TIMEOUT,
	ASR_UNKNOWN,
};

enum AsrResultType {
	// intermediate result of asr
	ASR_RES_INTER = 0,
	ASR_RES_START,
	// completed result of asr
	ASR_RES_END,
	ASR_RES_CANCELLED,
	ASR_RES_ERROR
};

struct AsrResult {
	// ASR_RES_INTER  intermediate result of asr
	// ASR_RES_START  stream result start
	// ASR_RES_END    completed result of asr
	// ASR_RES_CANCELLED  asr cancelled
	// ASR_RES_ERROR  asr occurs error, see AsrResult.err
	AsrResultType type;
	int32_t id;
	AsrError err;
	std::string asr;
};

class Asr {
public:
	virtual ~Asr() {}

	virtual bool prepare() = 0;

	virtual void release() = 0;

	virtual int32_t start() = 0;

	virtual void voice(int32_t id, const uint8_t* voice, uint32_t length) = 0;

	virtual void end(int32_t id) = 0;

	// param id:  > 0  cancel tts request specified by 'id'
	//           <= 0  cancel all tts requests
	virtual void cancel(int32_t id) = 0;

	// poll asr results
	// block current thread if no result available
	// if Asr.release() invoked, poll() will return -1
	//
	// return value  true  success
	//               false tts sdk released
	virtual bool poll(AsrResult& res) = 0;

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
	virtual void config(const char* key, const char* value) = 0;
};

std::shared_ptr<Asr> new_asr();

} // namespace speech
} // namespace rokid

