#pragma once

#include "asr.h"
#include "pending_queue.h"
#include "types.h"
#include "op_ctl.h"

namespace rokid {
namespace speech {

typedef OperationController<AsrStatus, AsrError> AsrOperationController;

class AsrImpl : public Asr {
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
	//       'key'                     'your_auth_key'
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

	void send_reqs();

	void gen_results();

	void gen_result_by_resp(rokid::open::AsrResponse& resp);

	bool gen_result_by_status();

	int32_t do_request(int32_t id, uint32_t type,
			std::shared_ptr<std::string>& voice, uint32_t err);

	bool do_ctl_change_op(int32_t id, uint32_t type);

private:
	int32_t next_id_;
	SpeechConfig config_;
	SpeechConnection connection_;
	StreamQueue<std::string> requests_;
	StreamQueue<std::string> responses_;
	std::mutex req_mutex_;
	std::condition_variable req_cond_;
	std::mutex resp_mutex_;
	std::condition_variable resp_cond_;
	AsrOperationController controller_;
	std::thread* req_thread_;
	std::thread* resp_thread_;
	bool initialized_;
};

} // namespace speech
} // namespace rokid
