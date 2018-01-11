#pragma once

#include <mutex>
#include <condition_variable>
#include <list>
#include <string>
#include <memory>
#include <thread>
#include "speech.h"
#include "types.h"
#include "op_ctl.h"
#include "pending_queue.h"
#include "speech_connection.h"
#include "nanopb_encoder.h"
#include "nanopb_decoder.h"

namespace rokid {
namespace speech {

typedef OperationController<SpeechStatus, SpeechError> SpeechOperationController;
typedef StreamQueue<std::string, VoiceOptions> ReqStreamQueue;
typedef StreamQueue<SpeechResultIn, int32_t> RespStreamQueue;

class SpeechOptionsHolder {
public:
	SpeechOptionsHolder();

	Lang lang;
	Codec codec;
	VadMode vad_mode;
	uint32_t vend_timeout;
	uint32_t no_nlp:1;
	uint32_t no_intermediate_asr:1;
	uint32_t unused:30;
};

class SpeechImpl : public Speech {
public:
	SpeechImpl();

	bool prepare(const PrepareOptions& options);

	void release();

	int32_t put_text(const char* text, const VoiceOptions* options);

	int32_t start_voice(const VoiceOptions* options);

	void put_voice(int32_t id, const uint8_t* data, uint32_t length);

	void end_voice(int32_t id);

	void cancel(int32_t id);

	bool poll(SpeechResult& res);

	void config(const std::shared_ptr<SpeechOptions>& options);

private:
	inline int32_t next_id() { return ++next_id_; }

	void send_reqs();

	void gen_results();

	void gen_result_by_resp(SpeechResponse& resp);

	bool gen_result_by_status();

	int32_t do_request(std::shared_ptr<SpeechReqInfo>& req);

	bool do_ctl_change_op(std::shared_ptr<SpeechReqInfo>& req);

	void req_config(SpeechRequest& req,
			const std::shared_ptr<VoiceOptions>& options);

	void erase_req(int32_t id);

#ifdef SPEECH_STATISTIC
	void finish_cur_req();
#endif

private:
	int32_t next_id_;
	SpeechOptionsHolder options_;
	SpeechConnection connection_;
	std::list<std::shared_ptr<SpeechReqInfo> > text_reqs_;
	ReqStreamQueue voice_reqs_;
	RespStreamQueue responses_;
	std::mutex req_mutex_;
	std::condition_variable req_cond_;
	std::mutex resp_mutex_;
	std::condition_variable resp_cond_;
	SpeechOperationController controller_;
	std::thread* req_thread_;
	std::thread* resp_thread_;
	bool initialized_;
#ifdef SPEECH_STATISTIC
	TraceInfo cur_trace_info_;
#endif
};

} // namespace speech
} // namespace rokid
