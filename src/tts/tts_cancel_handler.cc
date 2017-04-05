#include <assert.h>
#include "tts_cancel_handler.h"
#include "types.h"
#include "log.h"

using std::shared_ptr;

namespace rokid {
namespace speech {

int32_t TtsCancelHandler::handle_not_cancel(shared_ptr<TtsResult> in, void* arg) {
	assert(in.get());
	Log::d(tag__, "handle_not_cancel id %d, type %u", in->id, in->type);
	responses_.push_back(in);
	return 0;
}

shared_ptr<TtsResult> TtsCancelHandler::poll_not_block() {
	if (responses_.empty())
		return NULL;
	shared_ptr<TtsResult> r = responses_.front();
	responses_.pop_front();
	return r;
}

shared_ptr<TtsResult> TtsCancelHandler::generate_cancel_result(int32_t id) {
	shared_ptr<TtsResult> res(new TtsResult());
	res->id = id;
	res->type = 3;
	res->err = 0;
	return res;
}

} // namespace speech
} // namespace rokid
