#pragma once

#include <pthread.h>
#include <map>
#include <memory>
#include "pending_queue.h"
#include "log.h"

using std::map;

namespace rokid {
namespace speech {

template <typename T, typename C, typename QueueType>
class CallbackManager {
public:
	typedef map<int32_t, C*> MapType;

	CallbackManager() : queue_(NULL), next_id_(0) {
		pthread_mutex_init(&mutex_, NULL);
	}

	virtual ~CallbackManager() {
		pthread_mutex_destroy(&mutex_);
	}

	void prepare(QueueType* q) {
		queue_ = q;
		pthread_create(&thread_, NULL, callback_routine, this);
	}

	void close() {
		pthread_join(thread_, NULL);
	}

	int add_callback(C* cb) {
		int32_t id;
		pthread_mutex_lock(&mutex_);
		id = next_id();
		if (cb)
			callbacks_.insert(pair<int32_t, C*>(id, cb));
		pthread_mutex_unlock(&mutex_);
		return id;
	}

	void erase_callback(int32_t id) {
		pthread_mutex_lock(&mutex_);
		callbacks_.erase(id);
		pthread_mutex_unlock(&mutex_);
	}

	void clear() {
		pthread_mutex_lock(&mutex_);
		callbacks_.clear();
		pthread_mutex_unlock(&mutex_);
	}

protected:
	virtual void onData(C* cb, int32_t id, const T* data) = 0;

	virtual void callback_loop() = 0;

private:
	static void* callback_routine(void* arg) {
		CallbackManager* cb = (CallbackManager*)arg;
		cb->callback_loop();
		return NULL;
	}

	inline int32_t next_id() { return ++next_id_; }

protected:
	pthread_mutex_t mutex_;
	pthread_t thread_;
	QueueType* queue_;
	MapType callbacks_;

private:
	static const char* tag_;
	int32_t next_id_;
};

template <typename T, typename C, typename Q>
const char* CallbackManager<T, C, Q>::tag_ = "speech.CallbackManagerBase";

template <typename T, typename C>
class CallbackManagerStream : public CallbackManager<T, C, PendingStreamQueue<T> > {
public:
	virtual ~CallbackManagerStream() { }

protected:
	typedef PendingStreamQueue<T> QueueType;
	typedef map<int32_t, C*> MapType;

	void callback_loop() {
		uint32_t type;
		uint32_t err;
		int32_t id;
		int32_t cur_id = 0;
		shared_ptr<T> resp;
		typename MapType::iterator it;
		C* cb = NULL;

		while (true) {
			if (!this->queue_->poll(type, err, id, resp))
				break;
			switch (type) {
				case QueueType::QueueItem::uncompleted:
					assert(cb == NULL);
					pthread_mutex_lock(&this->mutex_);
					it = this->callbacks_.find(id);
					if (it != this->callbacks_.end())
						cb = it->second;
					pthread_mutex_unlock(&this->mutex_);
					Log::d(tag_, "poll response data, onStart %d, cb = %p", id, cb);
					if (cb) {
						cur_id = id;
						cb->onStart(id);
					}
					break;
				case QueueType::QueueItem::data:
					Log::d(tag_, "poll response data, onData %d, cb = %p", cur_id, cb);
					if (cb) {
						this->onData(cb, cur_id, resp.get());
					}
					break;
				case QueueType::QueueItem::completed:
					Log::d(tag_, "poll response data, onComplete %d, cb = %p", id, cb);
					if (cb) {
						// TODO:
						// tts request may occurs error
						// get error code from other way
						cb->onComplete(id);
						cb = NULL;
						this->erase_callback(id);
					}
					break;
				case QueueType::QueueItem::deleted:
					Log::d(tag_, "poll response data, onDelete %d, cb = %p", id, cb);
					if (cb) {
						cb->onStop(id);
						cb = NULL;
						this->erase_callback(id);
					}
					break;
				case QueueType::QueueItem::error:
					Log::d(tag_, "poll response data, onError %d:%d, cb = %p", id, err, cb);
					if (cb) {
						cb->onError(id, err);
						cb = NULL;
						this->erase_callback(id);
					}
					break;
			}
		}

		// thread quit
		// call onStop for all uncompleted callbacks
		pthread_mutex_lock(&this->mutex_);
		for (it = this->callbacks_.begin(); it != this->callbacks_.end(); ++it)
			it->second->onStop(it->first);
		pthread_mutex_unlock(&this->mutex_);
		Log::d(tag_, "callback thread quit");
	}

private:
	static const char* tag_;
};

template <typename T, typename C>
const char* CallbackManagerStream<T, C>::tag_ = "speech.CallbackManagerStream";

template <typename T, typename C>
class CallbackManagerUnary : public CallbackManager<T, C, PendingQueue<T> > {
protected:
	void callback_loop() {
		int id;
		shared_ptr<T> resp;
		typename map<int32_t, C*>::iterator it;
		C* cb;

		while (true) {
			if (!this->queue_->poll(id, resp))
				break;

			cb = NULL;
			pthread_mutex_lock(&this->mutex_);
			it = this->callbacks_.find(id);
			if (it != this->callbacks_.end())
				cb = it->second;
			pthread_mutex_unlock(&this->mutex_);

			if (cb) {
				this->onData(cb, id, resp.get());
				this->erase_callback(id);
			}
		}

	}
};

} // namespace speech
} // namespace rokid
