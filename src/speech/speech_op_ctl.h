#pragma once

#include <stdint.h>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include "types.h"
#include "speech.h"

namespace rokid {
namespace speech {

class SpeechOperatorController {
public:
	void new_op(int32_t id, SpeechStatus status);

	void set_op_error(SpeechError err, std::condition_variable& cond);

	void finish_op();

	void wait_op_finish(std::unique_lock<std::mutex>& locker);

	void cancel_op(int32_t id, std::condition_variable& cond);

public:
	int32_t id_;
	SpeechStatus status_;
	SpeechError error_;

private:
	std::condition_variable op_cond_;
};

} // namespace speech
} // namespace rokid
