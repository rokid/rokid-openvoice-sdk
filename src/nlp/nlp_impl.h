#pragma once

#include <string>
#include "nlp.h"
#include "pipeline.h"
#include "pending_queue.h"
#include "types.h"
#include "nlp_req_provider.h"
#include "nlp_req_handler.h"
#include "nlp_cancel_handler.h"

namespace rokid {
namespace speech {

class NlpImpl : public Nlp, public Pipeline<NlpReqInfo> {
public:
	NlpImpl();

	bool prepare();

	void release();

	int32_t request(const char* asr);

	void cancel(int32_t id);

	void config(const char* key, const char* value);

	bool poll(NlpResult& res);

private:
	int32_t next_id() { return ++next_id_; }

private:
	PendingQueue<std::string>* requests_;
	CommonArgument pipeline_arg_;
	NlpReqProvider req_provider_;
	NlpReqHandler req_handler_;
	NlpCancelHandler cancel_handler_;
	int32_t next_id_;
};

} // namespace speech
} // namespace rokid
