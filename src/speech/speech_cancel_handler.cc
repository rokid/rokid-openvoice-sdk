#include <assert.h>
#include "speech_cancel_handler.h"

using std::shared_ptr;

namespace rokid {
namespace speech {

int32_t SpeechCancelHandler::handle_not_cancel(shared_ptr<SpeechResult> in, void* arg) {
	assert(in.get());
	responses_.push_back(in);
	return 0;
}

shared_ptr<SpeechResult> SpeechCancelHandler::poll_not_block() {
	if (responses_.empty())
		return NULL;
	shared_ptr<SpeechResult> r = responses_.front();
	responses_.pop_front();
	return r;
}

shared_ptr<SpeechResult> SpeechCancelHandler::generate_cancel_result(int32_t id) {
	shared_ptr<SpeechResult> res(new SpeechResult());
	res->id = id;
	res->type = 3;
	res->err = 0;
	return res;
}

} // namespace speech
} // namespace rokid
