#pragma once

#include "tts.h"
#include "pipeline.h"
#include "tts_req_provider.h"
#include "tts_req_handler.h"
#include "tts_resp_handler.h"
#include "tts_cancel_handler.h"
#include "types.h"

namespace rokid {
namespace speech {

class TtsImpl : public Tts, public Pipeline<TtsReqInfo> {
public:
	TtsImpl();

	bool prepare();

	void release();

	int32_t speak(const char* text);

	void cancel(int32_t id);

	// poll tts results
	// block current thread if no result available
	// if Tts.release() invoked, poll() will return -1
	//
	// return value  true  success
	//               false tts sdk released
	bool poll(TtsResult& res);

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
	void config(const char* key, const char* value);

private:
	inline int32_t next_id() { return ++next_id_; }

private:
	TtsReqProvider req_provider_;
	TtsReqHandler req_handler_;
	TtsRespHandler resp_handler_;
	TtsCancelHandler cancel_handler_;
	CommonArgument pipeline_arg_;
	int32_t next_id_;
	PendingQueue<std::string>* requests_;
};

} // namespace speech
} // namespace rokid
