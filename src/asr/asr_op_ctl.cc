#include "asr_op_ctl.h"

namespace rokid {
namespace speech {

using std::unique_lock;
using std::mutex;
using std::condition_variable;

void AsrOperatorController::new_op(int32_t id, AsrStatus status) {
	id_ = id;
	status_ = status;
}

void AsrOperatorController::set_op_error(AsrError err,
		condition_variable& cond) {
	error_ = err;
	if (id_ > 0)
		cond.notify_one();
}

void AsrOperatorController::finish_op() {
	id_ = 0;
	op_cond_.notify_one();
}

void AsrOperatorController::wait_op_finish(unique_lock<mutex>& locker) {
	if (id_ > 0)
		op_cond_.wait(locker);
}

void AsrOperatorController::cancel_op(int32_t id,
		condition_variable& cond) {
	if (id <= 0 || id == id_) {
		status_ = ASR_STATUS_CANCELLED;
		Log::d(tag__, "ctl: cancel_op %d, Status --> Cancelled", id);
		if (id_ > 0)
			cond.notify_one();
	}
}

} // namespace speech
} // namespace rokid
