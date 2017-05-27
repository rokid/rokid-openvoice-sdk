#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "tts.h"
#include "speech_config.h"
#include "speech_connection.h"
#include "tts_op_ctl.h"
#include "types.h"
#include "speech.pb.h"

namespace rokid {
namespace speech {

class TtsImpl : public Tts {
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
	//       'key'                     'your_auth_key'
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

	void send_reqs();

	void gen_results();

	void gen_result_by_resp(rokid::open::TtsResponse& resp);

	bool gen_result_by_status();

	void do_request(std::shared_ptr<TtsReqInfo> req);

private:
	int32_t next_id_;
	SpeechConfig config_;
	SpeechConnection connection_;
	std::list<std::shared_ptr<TtsReqInfo> > requests_;
	std::list<std::shared_ptr<TtsResult> > responses_;
	std::mutex req_mutex_;
	std::condition_variable req_cond_;
	std::mutex resp_mutex_;
	std::condition_variable resp_cond_;
	TtsOperatorController controller_;
	std::thread* req_thread_;
	std::thread* resp_thread_;
	bool initialized_;
};

} // namespace speech
} // namespace rokid
