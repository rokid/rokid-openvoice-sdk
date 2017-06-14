#pragma once

#include <stdint.h>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <list>

#define NOOP_TIMEOUT 10000

namespace rokid {
namespace speech {

template <typename TStatus, typename TError>
class OperationController {
public:
	typedef struct {
		int32_t id;
		TStatus status;
		TError error;
		std::chrono::time_point<std::chrono::steady_clock> req_done_time;
		bool req_done;
	} Operation;

	void new_op(int32_t id, TStatus status) {
		std::shared_ptr<Operation> op(new Operation());
		op->id = id;
		op->status = status;
		op->req_done = false;
		operations_.push_back(op);
		if (status == TStatus::START)
			current_op_ = op;
	}

	// set current op error
	void set_op_error(TError err) {
		if (current_op_.get()) {
			current_op_->status = TStatus::ERROR;
			current_op_->error = err;
			current_op_.reset();
			op_cond_.notify_one();
		}
	}

	// finish current op
	void finish_op() {
		if (current_op_.get()) {
			if (current_op_->status != TStatus::CANCELLED
					&& current_op_->status != TStatus::ERROR)
				current_op_->status = TStatus::END;
			current_op_.reset();
			op_cond_.notify_one();
		}
	}

	// wait current op finish
	void wait_op_finish(int32_t id, std::unique_lock<std::mutex>& locker) {
		if (current_op_.get() && current_op_->id == id) {
			op_cond_.wait(locker);
		}
	}

	// cancel op specified by 'id'
	// if 'id' <= 0, cancel all operations
	void cancel_op(int32_t id, std::condition_variable& cond) {
		typename std::list<std::shared_ptr<Operation> >::iterator it;
		bool need_notify = false;
		for (it = operations_.begin(); it != operations_.end(); ++it) {
			if (id <= 0 || id == (*it)->id) {
				(*it)->status = TStatus::CANCELLED;
				if (current_op_.get() && current_op_->id == (*it)->id)
					need_notify = true;
				if (id > 0)
					break;
			}
		}
		if (need_notify)
			cond.notify_one();
	}

	void remove_front_op() {
		if (!operations_.empty())
			operations_.pop_front();
	}

	void req_done() {
		if (current_op_.get()) {
			current_op_->req_done = true;
			current_op_->req_done_time = std::chrono::steady_clock::now();
		}
	}

	uint32_t op_timeout() {
		if (current_op_.get() == NULL)
			return NOOP_TIMEOUT;
		if (!current_op_->req_done)
			return NOOP_TIMEOUT;
		std::chrono::duration<uint32_t, std::milli> dur =
			std::chrono::duration_cast<std::chrono::duration<uint32_t, std::milli> >
			(std::chrono::steady_clock::now() - current_op_->req_done_time);
		if (dur.count() > NOOP_TIMEOUT)
			return 0;
		return NOOP_TIMEOUT - dur.count();
	}

	std::shared_ptr<Operation>& current_op() {
		return current_op_;
	}

	std::shared_ptr<Operation>& front_op() {
		if (operations_.empty())
			return null_op_;
		return operations_.front();
	}

private:
	std::condition_variable op_cond_;
	std::list<std::shared_ptr<Operation> > operations_;
	std::shared_ptr<Operation> current_op_;
	std::shared_ptr<Operation> null_op_;
};

} // namespace speech
} // namespace rokid
