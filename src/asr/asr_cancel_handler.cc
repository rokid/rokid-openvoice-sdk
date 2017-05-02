#include <assert.h>
#include "asr_cancel_handler.h"

using std::shared_ptr;

namespace rokid {
namespace speech {

int32_t AsrCancelHandler::handle_not_cancel(shared_ptr<AsrResult> in, void* arg) {
	assert(in.get());
	responses_.push_back(in);
	return 0;
}

shared_ptr<AsrResult> AsrCancelHandler::poll_not_block() {
	if (responses_.empty())
		return NULL;
	shared_ptr<AsrResult> r = responses_.front();
	responses_.pop_front();
	return r;
}

shared_ptr<AsrResult> AsrCancelHandler::generate_cancel_result(int32_t id) {
	shared_ptr<AsrResult> res(new AsrResult());
	res->id = id;
	res->type = 3;
	res->err = AsrError::ASR_SUCCESS;
	return res;
}

} // namespace speech
} // namespace rokid
