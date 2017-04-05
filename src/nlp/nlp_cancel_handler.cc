#include <assert.h>
#include "nlp_cancel_handler.h"
#include "log.h"
#include "types.h"

using std::shared_ptr;

namespace rokid {
namespace speech {

int32_t NlpCancelHandler::handle_not_cancel(shared_ptr<NlpResult> in, void* arg) {
	assert(in.get());
	Log::d(tag__, "nlp cancel handler: put to response %d, %s",
			in->id, in->nlp.c_str());
	responses_.push_back(in);
	return 0;
}

shared_ptr<NlpResult> NlpCancelHandler::poll_not_block() {
	if (responses_.empty())
		return NULL;
	shared_ptr<NlpResult> res = responses_.front();
	responses_.pop_front();
	return res;
}

shared_ptr<NlpResult> NlpCancelHandler::generate_cancel_result(int32_t id) {
	shared_ptr<NlpResult> res(new NlpResult());
	res->id = id;
	res->err = 0;
	res->type = 1;
	return res;
}

} // namespace speech
} // namespace rokid
