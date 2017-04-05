#pragma once

#include <vector>
#include <list>
#include <thread>
#include <unistd.h>
#include "pipeline_handler.h"
#include "log.h"

namespace rokid {
namespace speech {

class PipelineWorkerInfo {
public:
	PipelineWorkerInfo() : handler_count_(0), thread_(NULL)
                         , begin_index_(0), end_index_(0) {
	}

	uint32_t handler_count_;
	std::thread* thread_;
	uint32_t begin_index_;
	uint32_t end_index_;
};

typedef struct {
	PipelineHandlerBase* handler;
	void* arg;
} PipelineHandlerInfo;

template <typename HeadType>
class Pipeline {
public:
	Pipeline(const char* tag) : head_(NULL), closed_(true), tag_(tag) {
	}

	void set_head(PipelineOutHandler<HeadType>* head) {
		if (!head_)
			head_ = head;
	}

	template <typename InType, typename OutType>
	bool add(PipelineHandler<InType, OutType>* handler, void* arg) {
		if (handlers_.empty()) {
			if (!head_)
				return false;
			handler->set_input(reinterpret_cast<PipelineOutHandler<InType>* >(head_));
		} else {
			PipelineOutHandler<InType>* prev = dynamic_cast<PipelineOutHandler<InType>*>(handlers_.back().handler);
			handler->set_input(prev);
		}
		PipelineHandlerInfo hinfo;
		hinfo.handler = handler;
		hinfo.arg = arg;
		handlers_.push_back(hinfo);
		return true;
	}

	void add_worker(uint32_t handler_count) {
		PipelineWorkerInfo new_worker;
		new_worker.handler_count_ = handler_count;
		workers_.push_back(new_worker);
	}

	void run() {
		closed_ = false;
		std::vector<PipelineWorkerInfo>::iterator it;
		uint32_t idx = 0;
		for (it = workers_.begin(); it != workers_.end(); ++it) {
			if (idx >= handlers_.size())
				break;
			uint32_t max_handlers = handlers_.size() - idx;
			if ((*it).handler_count_ == 0)
				(*it).handler_count_ = max_handlers;
			(*it).begin_index_ = idx;
			idx += (*it).handler_count_;
			(*it).end_index_ = idx;
			Log::d(tag_, "create worker thread, [%u, %u)", (*it).begin_index_, (*it).end_index_);
			(*it).thread_ = new std::thread([=] { do_work(*it); });
		}
	}

	void close() {
		closed_ = true;
		std::vector<PipelineWorkerInfo>::iterator it;
		for (it = workers_.begin(); it != workers_.end(); ++it) {
			if ((*it).thread_) {
				Log::d(tag_, "wait worker thread [%u, %u) exit", (*it).begin_index_, (*it).end_index_);
				(*it).thread_->join();
				delete (*it).thread_;
			}
		}
		Log::d(tag_, "all worker threads exit");
		workers_.clear();
		handlers_.clear();
		head_ = NULL;
	}

	inline bool closed() { return closed_; }


private:
	void do_work(const PipelineWorkerInfo& worker) {
		uint32_t i;
		int32_t r;
		std::list<uint32_t> idx_stack;

		while (!closed_) {
			if (idx_stack.empty())
				i = worker.begin_index_;
			else
				i = idx_stack.back();
			while (i < worker.end_index_) {
				r = handlers_[i].handler->do_work(handlers_[i].arg);
				if (r & FLAG_AS_HEAD) {
					if (idx_stack.empty() || i != idx_stack.back())
						idx_stack.push_back(i);
				} else {
					if (!idx_stack.empty() && i == idx_stack.back())
						idx_stack.pop_back();
				}
				if (r & FLAG_ERROR) {
					Log::d(tag_, "handler %d do work failed", i);
					sleep(1);
					break;
				}
				if (r & FLAG_BREAK_LOOP)
					break;
				++i;
			}
		}
		Log::d(tag_, "worker thread [%u, %u) exit", worker.begin_index_, worker.end_index_);
	}

private:
	PipelineOutHandler<HeadType>* head_;
	std::vector<PipelineHandlerInfo> handlers_;
	std::vector<PipelineWorkerInfo> workers_;
	bool closed_;
	// for debug
	const char* tag_;
};

} // namespace speech
} // namespace rokid
