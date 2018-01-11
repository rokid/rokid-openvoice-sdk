#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "tts.h"
#include "speech_connection.h"
#include "op_ctl.h"
#include "types.h"
#include "pending_queue.h"
#include "nanopb_decoder.h"

namespace rokid {
namespace speech {

typedef OperationController<TtsStatus, TtsError> TtsOperationController;
typedef StreamQueue<std::string, int32_t> TtsStreamQueue;

class TtsOptionsHolder {
public:
	TtsOptionsHolder();

	Codec codec;
	std::string declaimer;
	uint32_t samplerate;
};

class TtsImpl : public Tts {
public:
	TtsImpl();

	bool prepare(const PrepareOptions& options);

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

	void config(const std::shared_ptr<TtsOptions>& options);

private:
	inline int32_t next_id() { return ++next_id_; }

	void send_reqs();

	void gen_results();

	void gen_result_by_resp(TtsResponse& resp);

	bool gen_result_by_status();

	bool do_request(std::shared_ptr<TtsReqInfo>& req);

	TtsStatus do_ctl_new_op(std::shared_ptr<TtsReqInfo>& req);

#ifdef SPEECH_STATISTIC
	void finish_cur_req();
#endif

private:
	int32_t next_id_;
	TtsOptionsHolder options_;
	SpeechConnection connection_;
	std::list<std::shared_ptr<TtsReqInfo> > requests_;
	TtsStreamQueue responses_;
	std::mutex req_mutex_;
	std::condition_variable req_cond_;
	std::mutex resp_mutex_;
	std::condition_variable resp_cond_;
	TtsOperationController controller_;
	std::thread* req_thread_;
	std::thread* resp_thread_;
	bool initialized_;
#ifdef SPEECH_STATISTIC
	TraceInfo cur_trace_info_;
#endif
};

} // namespace speech
} // namespace rokid
