#pragma once

#include "asr.h"
#include "pipeline.h"
#include "types.h"
#include "asr_req_provider.h"
#include "asr_req_handler.h"
#include "asr_resp_handler.h"
#include "asr_cancel_handler.h"

namespace rokid {
namespace speech {

class AsrImpl : public Asr, public Pipeline<AsrReqInfo> {
public:
	AsrImpl();

	bool prepare();

	void release();

	int32_t start();

	void voice(int32_t id, const uint8_t* voice, uint32_t length);

	void end(int32_t id);

	// param id:  > 0  cancel tts request specified by 'id'
	//           <= 0  cancel all tts requests
	void cancel(int32_t id);

	// poll asr results
	// block current thread if no result available
	// if Asr.release() invoked, poll() will return -1
	//
	// return value  true  success
	//               false tts sdk released
	bool poll(AsrResult& res);

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
	void config(const char* key, const char* value);

private:
	inline int32_t next_id() { return ++next_id_; }

private:
	CommonArgument pipeline_arg_;
	AsrReqProvider req_provider_;
	AsrReqHandler req_handler_;
	AsrRespHandler resp_handler_;
	AsrCancelHandler cancel_handler_;
	PendingStreamQueue<std::string>* requests_;
	int32_t next_id_;
};

} // namespace speech
} // namespace rokid
