#include "tts_op_ctl.h"
#include "log.h"

namespace rokid {
namespace speech {

using std::unique_lock;
using std::mutex;
using std::condition_variable;

void TtsOperatorController::new_op(int32_t id, TtsStatus status) {
	id_ = id;
	status_ = status;
}

void TtsOperatorController::set_op_error(TtsError err,
		condition_variable& cond) {
	error_ = err;
	if (id_ > 0)
		cond.notify_one();
}

void TtsOperatorController::finish_op() {
	id_ = 0;
	op_cond_.notify_one();
}

void TtsOperatorController::wait_op_finish(unique_lock<mutex>& locker) {
	if (id_ > 0)
		op_cond_.wait(locker);
}

void TtsOperatorController::cancel_op(int32_t id,
		condition_variable& cond) {
	if (id <= 0 || id == id_) {
		Log::d(tag__, "ctl: cancel_op %d, Status --> Cancelled", id);
		status_ = TTS_STATUS_CANCELLED;
		if (id_ > 0)
			cond.notify_one();
	}
}

} // namespace speech
} // namespace rokid
