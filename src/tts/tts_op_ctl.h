#pragma once

#include <stdint.h>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include "types.h"
#include "tts.h"

namespace rokid {
namespace speech {

class TtsOperatorController {
public:
	void new_op(int32_t id, TtsStatus status);

	void set_op_error(TtsError err, std::condition_variable& cond);

	void finish_op();

	void wait_op_finish(std::unique_lock<std::mutex>& locker);

	void cancel_op(int32_t id, std::condition_variable& cond);

public:
	int32_t id_;
	TtsStatus status_;
	TtsError error_;

private:
	std::condition_variable op_cond_;
};

} // namespace speech
} // namespace rokid
