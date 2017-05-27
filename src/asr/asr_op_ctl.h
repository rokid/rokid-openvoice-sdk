#pragma once

#include <stdint.h>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include "types.h"
#include "asr.h"

namespace rokid {
namespace speech {

class AsrOperatorController {
public:
	void new_op(int32_t id, AsrStatus status);

	void set_op_error(AsrError err, std::condition_variable& cond);

	void finish_op();

	void wait_op_finish(std::unique_lock<std::mutex>& locker);

	void cancel_op(int32_t id, std::condition_variable& cond);

public:
	int32_t id_;
	AsrStatus status_;
	AsrError error_;

private:
	std::condition_variable op_cond_;
};

} // namespace speech
} // namespace rokid
