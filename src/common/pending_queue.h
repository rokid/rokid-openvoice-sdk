#pragma once

#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <list>
#include <map>
#include <memory>
#include "log.h"

using namespace std;

namespace rokid {
namespace speech {

template <typename P>
class PendingQueueItem;

template <typename T>
class PendingQueue {
public:
	typedef shared_ptr<T> T_sp;

	class QueueItem {
	public:
		int32_t id;
		bool deleted;
		T_sp data;
	};

	typedef shared_ptr<QueueItem> QueueItemSp;

	PendingQueue() : closed_(false) {
		pthread_mutex_init(&mutex_, NULL);
		pthread_cond_init(&cond_, NULL);
	}

	~PendingQueue() {
		pthread_mutex_destroy(&mutex_);
		pthread_cond_destroy(&cond_);
	}

	bool add(int32_t id, T_sp data) {
		pthread_mutex_lock(&mutex_);
		if (closed_) {
			pthread_mutex_unlock(&mutex_);
			return false;
		}
		QueueItemSp item(new QueueItem());
		item->id = id;
		item->deleted = false;
		item->data = data;
		queue_.push_back(item);
		pthread_cond_signal(&cond_);
		pthread_mutex_unlock(&mutex_);
		return true;
	}

	bool erase(int32_t id) {
		typename list<QueueItemSp>::iterator it;
		bool r = false;

		pthread_mutex_lock(&mutex_);
		for (it = queue_.begin(); it != queue_.end(); ++it) {
			if ((*it)->id == id) {
				(*it)->deleted = true;
				r = true;
				break;
			}
		}
		pthread_mutex_unlock(&mutex_);
		return r;
	}

	void clear(int32_t* min_id, int32_t* max_id) {
		int32_t min = 0;
		int32_t max = 0;
		typename list<QueueItemSp>::iterator it;

		pthread_mutex_lock(&mutex_);
		if (queue_.size()) {
			min = queue_.front()->id;
			max = queue_.back()->id;
		}
		for (it = queue_.begin(); it != queue_.end(); ++it)
			(*it)->deleted = true;
		pthread_mutex_unlock(&mutex_);
		if (min_id)
			*min_id = min;
		if (max_id)
			*max_id = max;
	}

	void close() {
		pthread_mutex_lock(&mutex_);
		queue_.clear();
		closed_ = true;
		pthread_cond_signal(&cond_);
		pthread_mutex_unlock(&mutex_);
	}

	void reset() {
		closed_ = false;
	}

	uint32_t size() {
		uint32_t s;
		pthread_mutex_lock(&mutex_);
		s = queue_.size();
		pthread_mutex_unlock(&mutex_);
		return s;
	}

	inline bool closed() { return closed_; }

	bool poll(int32_t& id, T_sp& data, bool& del) {
		bool r;
		QueueItemSp item;

		pthread_mutex_lock(&mutex_);
		if (!closed_ && queue_.empty())
			pthread_cond_wait(&cond_, &mutex_);
		if (!queue_.empty()) {
			item = queue_.front();
			id = item->id;
			data = item->data;
			del = item->deleted;
			queue_.pop_front();
			r = true;
		} else {
			// 'close' invoked
			assert(closed_);
			r = false;
		}
		pthread_mutex_unlock(&mutex_);
		return r;
	}

protected:
	list<QueueItemSp> queue_;
	pthread_mutex_t mutex_;
	pthread_cond_t cond_;
	bool closed_;
}; // class PendingQueue

#define STREAM_QUEUE_TAG "speech.StreamQueue"

template <typename T, typename A>
class StreamQueue {
public:
	typedef shared_ptr<T> T_sp;
	typedef shared_ptr<A> A_sp;

	class QueueItem {
	public:
		enum ItemType {
			data,
			uncompleted,
			completed,
			deleted,
			error,
		};

		QueueItem() : type(0), polling(0), err(0), id(0) {
		}

		// 0: data
		// 1: tag (stream uncompleted)
		// 2: tag (stream completed)
		// 3: tag (stream deleted)
		// 4: tag (stream error)
		uint32_t type:15;
		// stream uncompleted and is polling
		uint32_t polling:1;
		uint32_t err:16;
		// only meaningful when 'type' not 'data'
		int32_t id;
		// only meaningful when 'type' is 'data' or 'completed'
		T_sp content;
		// only meaningful when 'type' not 'data'
		A_sp arg;
		uint32_t data_count;
	};

	enum PopType {
		POP_TYPE_EMPTY = -1,
		POP_TYPE_DATA,
		POP_TYPE_START,
		POP_TYPE_END,
		POP_TYPE_REMOVED,
		POP_TYPE_ERROR
	};

	typedef shared_ptr<QueueItem> QueueItemSp;
	typedef typename list<QueueItemSp>::iterator StreamingItemPos;

	bool start(int32_t id) {
		if (item_tags_.find(id) != item_tags_.end()) {
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
			Log::i(STREAM_QUEUE_TAG, "add tag for %d failed, "
					"already existed", id);
#endif
			return false;
		}

		QueueItemSp item(new QueueItem());
		item->type = QueueItem::uncompleted;
		item->polling = 0;
		item->id = id;
		item->data_count = 0;
		StreamingItemPos it;
		it = queue_.insert(queue_.end(), item);
		item_tags_.insert(pair<int, StreamingItemPos>(item->id, it));
		tag_queue_.push_back(it);
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
		Log::d(STREAM_QUEUE_TAG, "add tag for id %d", id);
#endif
		return true;
	}

	bool stream(int32_t id, T_sp data) {
		typename map<int, StreamingItemPos>::iterator it;
		it = item_tags_.find(id);
		if (it == item_tags_.end()
				|| (*it->second)->type != QueueItem::uncompleted) {
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
			if (it == item_tags_.end())
				Log::i(STREAM_QUEUE_TAG, "add data for id %d failed, "
						"the tag not existed", id);
			else
				Log::i(STREAM_QUEUE_TAG, "add data for id %d failed, "
						"the tag type is %d", id, (*it->second)->type);
#endif
			return false;
		}

		QueueItemSp item(new QueueItem());
		item->type = QueueItem::data;
		item->content = data;
		queue_.insert(it->second, item);
		++(*it->second)->data_count;
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
		Log::d(STREAM_QUEUE_TAG, "add data for id %d, "
				"data count is %d", id,
				(*it->second)->data_count);
#endif
		return true;
	}

	bool end(int32_t id, T_sp data = NULL) {
		typename map<int32_t, StreamingItemPos>::iterator it;

		it = item_tags_.find(id);
		if (it == item_tags_.end()) {
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
			Log::i(STREAM_QUEUE_TAG, "complete tag for id %d failed, "
					"tag not existed", id);
#endif
			return false;
		}
		(*it->second)->type = QueueItem::completed;
		if (data.get())
			(*it->second)->content = data;
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
		Log::d(STREAM_QUEUE_TAG, "complete tag for id %d, "
				"data count is %d", id, (*it->second)->data_count);
#endif
		return true;
	}

	void set_arg(int32_t id, A_sp& arg) {
		typename map<int32_t, StreamingItemPos>::iterator it;

		it = item_tags_.find(id);
		if (it == item_tags_.end()) {
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
			Log::i(STREAM_QUEUE_TAG, "set_arg for id %d failed, "
					"tag not existed", id);
#endif
			return;
		}
		(*it->second)->arg = arg;
	}

	A_sp get_arg(int32_t id) {
		typename map<int32_t, StreamingItemPos>::iterator it;

		it = item_tags_.find(id);
		if (it == item_tags_.end()) {
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
			Log::i(STREAM_QUEUE_TAG, "get_arg for id %d failed, "
					"tag not existed", id);
#endif
			return NULL;
		}
		return (*it->second)->arg;
	}

	uint32_t size() {
		return item_tags_.size();
	}

	bool erase(int32_t id, uint32_t err = 0) {
		typename map<int32_t, StreamingItemPos>::iterator it;
		typename list<QueueItemSp>::iterator first_it;
		typename list<QueueItemSp>::iterator last_it;
		uint32_t c = 0;

		it = item_tags_.find(id);
		if (it != item_tags_.end()) {
			assert(id == (*it->second)->id);
			last_it = it->second;
			first_it = last_it;
			if (first_it != queue_.begin()) {
				do {
					--first_it;
					if ((*first_it)->type != QueueItem::data) {
						++first_it;
						break;
					}
					++c;
				} while (first_it != queue_.begin());
			}
			assert((*last_it)->type != QueueItem::data);
			(*last_it)->content.reset();
			if (err) {
				(*last_it)->type = QueueItem::error;
				(*last_it)->err = err;
			} else
				(*last_it)->type = QueueItem::deleted;
			if (first_it != last_it) {
				(*last_it)->data_count -= c;
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
				Log::d(STREAM_QUEUE_TAG, "erase %d data for id %d, "
						"data count is %d, err %d", c, id,
						(*last_it)->data_count, err);
#endif
				queue_.erase(first_it, last_it);
			}
			return true;
		}
		return false;
	}

	void clear(int32_t* min_id, int32_t* max_id) {
		typename list<QueueItemSp>::iterator it;
		typename list<QueueItemSp>::iterator tmpit;
		int32_t min = 0;
		int32_t max = 0;
		uint32_t c = 0;

		if (tag_queue_.size()) {
			min = (*tag_queue_.front())->id;
			max = (*tag_queue_.back())->id;
		}
		it = queue_.begin();
		while (it != queue_.end()) {
			if ((*it)->type == QueueItem::data) {
				tmpit = it;
				++it;
				queue_.erase(tmpit);
				++c;
			} else {
				(*it)->type = QueueItem::deleted;
				(*it)->content.reset();
				(*it)->data_count -= c;
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
				Log::d(STREAM_QUEUE_TAG, "clear queue, erase %d data "
						"for id %d, finally count is %d", c,
						(*it)->id, (*it)->data_count);
#endif
				++it;
			}
		}
		if (min_id)
			*min_id = min;
		if (max_id)
			*max_id = max;
	}

	void close() {
		queue_.clear();
		item_tags_.clear();
		tag_queue_.clear();
	}

	bool available() {
		if (tag_queue_.empty())
			return false;
		StreamingItemPos ip = tag_queue_.front();
		QueueItemSp item = *ip;
		assert(item->type != QueueItem::data);
		if (item->type == QueueItem::uncompleted
				&& item->polling) {
			// if ip == queue_.begin(),
			// the stream no data available now,
			// not end, wait for more data,
			// should return false.
			return ip != queue_.begin();
		}
		return true;
	}

	int32_t pop(int32_t& id, T_sp& res, uint32_t& err) {
		if (tag_queue_.empty()) {
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
			Log::d(STREAM_QUEUE_TAG, "pop return EMPTY");
#endif
			return POP_TYPE_EMPTY;
		}
		StreamingItemPos ip = tag_queue_.front();
		QueueItemSp item = *ip;
		assert(item->type != QueueItem::data);
		if (item->type == QueueItem::uncompleted
				|| item->type == QueueItem::completed) {
			if (!item->polling) {
				id = item->id;
				item->polling = 1;
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
				Log::d(STREAM_QUEUE_TAG, "pop return start for id %d, "
						"data count %d", id, item->data_count);
#endif
				return POP_TYPE_START;
			}
			if (ip == queue_.begin()) {
				if (item->type == QueueItem::uncompleted) {
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
					Log::d(STREAM_QUEUE_TAG, "pop return EMPTY, "
							"id %d, data count = %d", item->id,
							item->data_count);
#endif
					// if ip == queue_.begin(),
					// the stream no data available now,
					// not end, wait for more data.
					return POP_TYPE_EMPTY;
				}
				id = item->id;
				if (item->content.get())
					res = item->content;
				item_tags_.erase(item->id);
				tag_queue_.pop_front();
				queue_.pop_front();
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
				Log::d(STREAM_QUEUE_TAG, "pop return complete for "
						"id %d, data count %d", item->id,
						item->data_count);
#endif
				return POP_TYPE_END;
			}
			id = item->id;
			--item->data_count;
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
			Log::d(STREAM_QUEUE_TAG, "pop return data for id %d, "
					"data count %d", item->id, item->data_count);
#endif
			item = queue_.front();
			assert(item->type == QueueItem::data);
			queue_.pop_front();
			res = item->content;
			return POP_TYPE_DATA;
		} else if (item->type == QueueItem::deleted) {
			id = item->id;
			item_tags_.erase(item->id);
			tag_queue_.pop_front();
			queue_.pop_front();
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
			Log::d(STREAM_QUEUE_TAG, "pop return deleted for id %d, "
					"data count %d", item->id, item->data_count);
#endif
			return POP_TYPE_REMOVED;
		}
		id = item->id;
		err = item->err;
		item_tags_.erase(item->id);
		tag_queue_.pop_front();
		queue_.pop_front();
#ifdef SPEECH_SDK_STREAM_QUEUE_TRACE
		Log::d(STREAM_QUEUE_TAG, "pop return error for id %d, "
				"data count %d", item->id, item->data_count);
#endif
		return POP_TYPE_ERROR;
	}

protected:
	list<QueueItemSp> queue_;
	list<StreamingItemPos> tag_queue_;
	map<int32_t, StreamingItemPos> item_tags_;
}; // class StreamQueue

template <typename T, typename A>
class PendingStreamQueue : public StreamQueue<T, A> {
public:
	typedef StreamQueue<T, A> BaseQueue;
	using typename BaseQueue::QueueItem;
	using BaseQueue::queue_;
	using BaseQueue::tag_queue_;
	using BaseQueue::item_tags_;
	typedef shared_ptr<T> T_sp;
	typedef shared_ptr<QueueItem> QueueItemSp;
	typedef typename list<QueueItemSp>::iterator StreamingItemPos;

	PendingStreamQueue() : closed_(false) {
		pthread_mutex_init(&mutex_, NULL);
		pthread_cond_init(&cond_, NULL);
	}

	~PendingStreamQueue() {
		pthread_mutex_destroy(&mutex_);
		pthread_cond_destroy(&cond_);
	}

	bool start(int32_t id) {
		pthread_mutex_lock(&mutex_);
		if (closed_) {
			pthread_mutex_unlock(&mutex_);
			return false;
		}
		if (BaseQueue::start(id)) {
			pthread_cond_signal(&cond_);
		}
		pthread_mutex_unlock(&mutex_);
		return true;
	}

	void stream(int32_t id, T_sp data) {
		pthread_mutex_lock(&mutex_);
		if (closed_) {
			pthread_mutex_unlock(&mutex_);
			return;
		}
		if (BaseQueue::stream(id, data)) {
			pthread_cond_signal(&cond_);
		}
		pthread_mutex_unlock(&mutex_);
	}

	void end(int32_t id) {
		pthread_mutex_lock(&mutex_);
		if (closed_) {
			pthread_mutex_unlock(&mutex_);
			return;
		}
		if (BaseQueue::end(id)) {
			pthread_cond_signal(&cond_);
		}
		pthread_mutex_unlock(&mutex_);
	}

	bool erase(int32_t id, uint32_t err = 0) {
		bool r = false;
		pthread_mutex_lock(&mutex_);
		if (closed_) {
			pthread_mutex_unlock(&mutex_);
			return false;
		}
		if (BaseQueue::erase(id, err)) {
			r = true;
			pthread_cond_signal(&cond_);
		}
		pthread_mutex_unlock(&mutex_);
		return r;
	}

	void clear(int32_t* min_id, int32_t* max_id) {
		int32_t min;
		int32_t max;
		pthread_mutex_lock(&mutex_);
		BaseQueue::clear(&min, &max);
		if (min > 0)
			pthread_cond_signal(&cond_);
		pthread_mutex_unlock(&mutex_);
		if (min_id)
			*min_id = min;
		if (max_id)
			*max_id = max;
	}

	void close() {
		pthread_mutex_lock(&mutex_);
		BaseQueue::close();
		closed_ = true;
		pthread_cond_signal(&cond_);
		pthread_mutex_unlock(&mutex_);
	}

	void reset() {
		closed_ = false;
	}

	uint32_t size() {
		uint32_t s;
		pthread_mutex_lock(&mutex_);
		s = BaseQueue::size();
		pthread_mutex_unlock(&mutex_);
		return s;
	}

	inline bool closed() { return closed_; }

	// return  true: success
	//         false: queue closed
	// param 'id'  item id: if 'type != 0'
	//                   0: if 'type' == 0
	// param 'type' 0:  data
	//              1:  stream start
	//              2:  stream end
	//              3:  stream deleted
	bool poll(uint32_t& type, uint32_t& err, int32_t& id, T_sp& item) {
		int32_t r;
		pthread_mutex_lock(&mutex_);
		do {
			r = this->pop(id, item, err);
			if (r < 0) {
				pthread_cond_wait(&cond_, &mutex_);
				if (closed_) {
					pthread_mutex_unlock(&mutex_);
					return false;
				}
			}
		} while (r < 0);
		type = r;
		pthread_mutex_unlock(&mutex_);
		return true;
	}

private:
	pthread_mutex_t mutex_;
	pthread_cond_t cond_;
	bool closed_;
}; // class PendingStreamQueue

} // namespace speech
} // namespace rokid
