#pragma once

#include <list>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "pipeline_handler.h"

namespace rokid {
namespace speech {

typedef struct {
	int32_t id;
	bool notified;
} CancelInfo;

template <typename InType, typename OutType>
class CancelPipelineHandler : public PipelineHandler<InType, OutType> {
public:
	CancelPipelineHandler() : closed_(false) {
	}

	virtual ~CancelPipelineHandler() {}

	void cancel(int32_t id) {
		CancelInfo c;
		c.id = id;
		c.notified = false;
		std::lock_guard<std::mutex> locker(mutex_);
		cancel_infos_.push_back(c);
	}

	void cancelled(int32_t id) {
		std::lock_guard<std::mutex> locker(mutex_);
		cancelled_.push_back(id);
		cond_.notify_one();
	}

	std::shared_ptr<OutType> poll() {
		std::unique_lock<std::mutex> locker(mutex_);
		while (!closed_) {
			if (!cancelled_.empty()) {
				std::shared_ptr<OutType> r = generate_cancel_result(cancelled_.front());
				cancelled_.pop_front();
				return r;
			}
			std::shared_ptr<OutType> res = poll_not_block();
			if (res.get() != NULL)
				return res;
			cond_.wait(locker);
		}
		return NULL;
	}

	void close() {
		std::lock_guard<std::mutex> locker(mutex_);
		closed_ = true;
		cond_.notify_one();
	}

	bool closed() {
		return closed_;
	}

protected:
	virtual int32_t handle_not_cancel(std::shared_ptr<InType> in, void* arg) = 0;

	virtual std::shared_ptr<OutType> poll_not_block() = 0;

	virtual std::shared_ptr<OutType> generate_cancel_result(int32_t id) = 0;

private:
	void start_handle(std::shared_ptr<InType> in, void* arg) {
		// do nothing
	}

	int32_t handle(std::shared_ptr<InType> in, void* arg) {
		if (!in.get())
			return 0;
		CancelInfo* c = is_cancel(in->id);
		if (c) {
			if (!c->notified) {
				cancelled(in->id);
				c->notified = true;
			}
			return 0;
		}

		std::lock_guard<std::mutex> locker(mutex_);
		int32_t r = handle_not_cancel(in, arg);
		cond_.notify_one();
		return r;
	}

	void end_handle(std::shared_ptr<InType> in, void* arg) {
		// do nothing
	}

	CancelInfo* is_cancel(int32_t id) {
		std::list<CancelInfo>::iterator it;
		std::lock_guard<std::mutex> locker(mutex_);
		it = cancel_infos_.begin();
		while (it != cancel_infos_.end()) {
			if ((*it).id == id)
				return &(*it);
			// the cancel info expired, remove it
			if (id > (*it).id) {
				cancel_infos_.erase(it);
				continue;
			}
			++it;
		}
		return NULL;
	}

private:
	std::list<CancelInfo> cancel_infos_;
	std::list<int32_t> cancelled_;
	std::mutex mutex_;
	std::condition_variable cond_;
	bool closed_;
};

} // namespace speech
} // namespace rokid
