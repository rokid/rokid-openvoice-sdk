#pragma once

#include <pthread.h>
#include "pending_queue.h"
#include "log.h"

namespace rokid {
namespace speech {

// 2 threads
// rpc param not stream, return is stream
template <typename S, typename R>
class GrpcRunnerBase {
public:
	typedef shared_ptr<S> S_sp;
	typedef GrpcRunnerBase<S, R> BaseType;

	class ThreadParam {
	public:

		BaseType* holder;
		void (BaseType::*func)();
	};

	GrpcRunnerBase() : id_for_response_(0), resp_work_(true) {
		pthread_mutex_init(&stream_mutex_, NULL);
		pthread_cond_init(&stream_cond_, NULL);

		responses_ = new PendingStreamQueue<R>();
	}

	virtual ~GrpcRunnerBase() {
		pthread_mutex_destroy(&stream_mutex_);
		pthread_cond_destroy(&stream_cond_);

		delete responses_;
	}

	virtual bool prepare() {
		ThreadParam* arg = new ThreadParam();
		arg->holder = this;
		arg->func = &BaseType::run_request;
		pthread_create(&req_thread_, NULL, thread_routine, arg);
		arg = new ThreadParam();
		arg->holder = this;
		arg->func = &BaseType::run_response;
		pthread_create(&resp_thread_, NULL, thread_routine, arg);
		return true;
	}

	void close() {
		pthread_mutex_lock(&stream_mutex_);
		resp_work_ = false;
		pthread_cond_signal(&stream_cond_);
		pthread_mutex_unlock(&stream_mutex_);
		responses_->close();

		pthread_join(req_thread_, NULL);
		pthread_join(resp_thread_, NULL);
	}

	inline PendingStreamQueue<R>* responses() {
		return responses_;
	}

protected:
	virtual void do_stream_response(int32_t id, S_sp stream, PendingStreamQueue<R>* responses) = 0;

	virtual void run_request() = 0;

private:
	void run_response() {
		S_sp stream;

		Log::d(tag_, "run_response thread start");
		while (true) {
			pthread_mutex_lock(&stream_mutex_);
			if (stream_for_response_.get() == NULL) {
				pthread_cond_wait(&stream_cond_, &stream_mutex_);
				// request PendingQueue closed, stop work
				if (stream_for_response_.get() == NULL) {
					pthread_mutex_unlock(&stream_mutex_);
					break;
				}
			}
			stream = stream_for_response_;
			stream_for_response_.reset();
			pthread_mutex_unlock(&stream_mutex_);

			do_stream_response(id_for_response_, stream, responses_);

			// awake request thread, do next request
			pthread_mutex_lock(&stream_mutex_);
			pthread_cond_signal(&stream_cond_);
			if (!resp_work_) {
				pthread_mutex_unlock(&stream_mutex_);
				break;
			}
			pthread_mutex_unlock(&stream_mutex_);
		}
		Log::d(tag_, "run_response thread quit");
	}

	static void* thread_routine(void* arg) {
		ThreadParam* tp = (ThreadParam*)arg;
		// void (BaseType::*f)() = tp->func;
		// BaseType* holder = tp->holder;
		(tp->holder->*tp->func)();
		delete tp;
		return NULL;
	}

protected:
	static const char* tag_;

	pthread_mutex_t stream_mutex_;
	pthread_cond_t stream_cond_;
	S_sp requesting_stream_;
	S_sp stream_for_response_;
	int id_for_response_;
	PendingStreamQueue<R>* responses_;

private:
	pthread_t req_thread_;
	pthread_t resp_thread_;
	bool resp_work_;
};

template <typename S, typename R>
const char* GrpcRunnerBase<S, R>::tag_ = "speech.GrpcRunnerBase";

template <typename Q, typename S, typename R>
class GrpcRunner : public GrpcRunnerBase<S, R> {
public:
	typedef shared_ptr<Q> Q_sp;
	typedef shared_ptr<S> S_sp;

	GrpcRunner() {
		requests_ = new PendingQueue<Q>();
	}

	~GrpcRunner() {
		delete requests_;
	}

	inline PendingQueue<Q>* requests() {
		return requests_;
	}

	void cancel(int32_t id) {
		requests_->erase(id);
		this->responses_->erase(id);
		// TODO: responses of this id may not clear
	}

	void clear() {
		requests_->clear();
		this->responses_->clear();
	}

	void close() {
		requests_->close();
		GrpcRunnerBase<S, R>::close();
	}

protected:
	virtual S_sp do_request(int32_t id, Q_sp data) = 0;

	void run_request() {
		Q_sp data;
		S_sp stream;
		int32_t id;

		while (true) {
			if (!requests_->poll(id, data))
				// request PendingQueue closed, stop work
				break;

			assert(id > 0);
			stream = do_request(id, data);
			if (stream.get() == NULL)
				continue;

			// awake response thread
			pthread_mutex_lock(&this->stream_mutex_);
			assert(this->stream_for_response_ == NULL);
			this->stream_for_response_ = stream;
			this->id_for_response_ = id;
			pthread_cond_signal(&this->stream_cond_);
			// wait response read completed
			pthread_cond_wait(&this->stream_cond_, &this->stream_mutex_);
			pthread_mutex_unlock(&this->stream_mutex_);
		}
		Log::d(this->tag_, "run_request thread quit");
	}

private:
	PendingQueue<Q>* requests_;
};

// 2 threads
// rpc param and return are stream both
template <typename Q, typename S, typename R>
class GrpcRunnerStream : public GrpcRunnerBase<S, R> {
public:
	typedef shared_ptr<Q> Q_sp;
	typedef shared_ptr<S> S_sp;
	typedef PendingStreamQueue<Q> QQueue;

	GrpcRunnerStream() {
		requests_ = new QQueue();
	}

	~GrpcRunnerStream() {
		delete requests_;
	}

	inline QQueue* requests() { return requests_; }

	void cancel(int id) {
		requests_->erase(id);
		this->responses_->erase(id);
	}

	void clear() {
		requests_->clear();
		this->responses_->clear();
	}

	void close() {
		requests_->close();
		GrpcRunnerBase<S, R>::close();
	}

protected:
	virtual S_sp do_stream_start(int32_t id) = 0;

	virtual void do_stream_data(int32_t id, S_sp stream, Q_sp data) = 0;

	virtual void do_stream_end(int32_t id, S_sp stream) = 0;

	void run_request() {
		uint32_t type;
		uint32_t err;
		Q_sp data;
		int32_t id;
		int32_t cur_id = 0;
		S_sp stream;

		while (true) {
			if (!requests_->poll(type, err, id, data))
				// queue closed, quit
				break;

			if (type != QQueue::QueueItem::data)
				cur_id = id;

			// assert requesting_stream_ != null ?
			if (type == QQueue::QueueItem::data)
				do_stream_data(cur_id, stream, data);
			else if (type == QQueue::QueueItem::uncompleted) {
				stream = do_stream_start(cur_id);
				// awake response thread
				assert(this->stream_for_response_ == NULL);
				pthread_mutex_lock(&this->stream_mutex_);
				this->stream_for_response_ = stream;
				this->id_for_response_ = cur_id;
				pthread_cond_signal(&this->stream_cond_);
				pthread_mutex_unlock(&this->stream_mutex_);
			} else
				// type is 'completed' or 'deleted'
				do_stream_end(cur_id, stream);
		}
	}

private:
	QQueue* requests_;
};

// single thread
// rpc param and return not stream both
template <typename Q, typename R>
class GrpcRunnerSync {
public:
	typedef shared_ptr<Q> Q_sp;

	GrpcRunnerSync() {
		requests_ = new PendingQueue<Q>();
		responses_ = new PendingQueue<R>();
	}

	virtual ~GrpcRunnerSync() {
		delete requests_;
		delete responses_;
	}

	inline PendingQueue<Q>* requests() { return requests_; }

	inline PendingQueue<R>* responses() { return responses_; }

	void cancel(int32_t id) {
		requests_->erase(id);
		responses_->erase(id);
	}

	void clear() {
		requests_->clear();
		responses_->clear();
	}

	void close() {
		requests_->close();
		responses_->close();
		pthread_join(thread_, NULL);
	}

	virtual bool prepare() {
		pthread_create(&thread_, NULL, thread_routine, this);
		return true;
	}

protected:
	virtual void do_request(int32_t id, Q_sp data) = 0;

private:
	static void* thread_routine(void* arg) {
		GrpcRunnerSync* runner = (GrpcRunnerSync*)arg;
		runner->run();
		return NULL;
	}

	void run() {
		Q_sp data;
		int32_t id;

		while (true) {
			if (!requests_->poll(id, data))
				// request PendingQueue closed, stop work
				break;
			do_request(id, data);
		}
		Log::d(tag_, "runner thread quit");
	}

private:
	static const char* tag_;

	pthread_t thread_;
	PendingQueue<Q>* requests_;
	PendingQueue<R>* responses_;
};

template <typename Q, typename R>
const char* GrpcRunnerSync<Q, R>::tag_ = "speech.GrpcRunnerSync";

} // namespace speech
} // namespace rokid
