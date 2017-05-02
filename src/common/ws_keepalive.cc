#include <chrono>
#include "ws_keepalive.h"

#define SOCKET_BUF_SIZE 0x40000
#define SOCKET_TIMEOUT 20
#define KA_TAG "speech.KeepAlive"

using std::shared_ptr;
using std::lock_guard;
using std::unique_lock;
using std::chrono::duration;
using std::mutex;
using std::thread;

namespace rokid {
namespace speech {

WSKeepAlive::WSKeepAlive() : work_(false) {
}

void WSKeepAlive::start(uint32_t interval_time, SpeechConfig* config,
		const char* svc) {
	Log::d(KA_TAG, "start keepalive thread for service %s, interval %u",
			svc, interval_time);
	interval_time_ = interval_time;
	config_ = config;
	service_ = svc;
	work_ = true;
	thread_ = new thread([=] { run(); });
}

void WSKeepAlive::close() {
	if (thread_) {
		unique_lock<mutex> locker(mutex_);
		work_ = false;
		shutdown_queue_.clear();
		cond_.notify_one();
		conn_ready_.notify_one();
		locker.unlock();
		thread_->join();
		delete thread_;
		thread_ = NULL;

		if (connection_.get()) {
			connection_->close();
			connection_.reset();
		}
	}
}

void WSKeepAlive::shutdown(SpeechConnection* conn) {
	Log::d(KA_TAG, "shutdown request for connection %u",
			conn->get_id());
	lock_guard<mutex> locker(mutex_);
	shutdown_queue_.push_back(conn->get_id());
	cond_.notify_one();
}

shared_ptr<SpeechConnection> WSKeepAlive::get_conn(uint32_t timeout) {
	unique_lock<mutex> locker(mutex_);
	if (work_ && !connection_.get()) {
		Log::d(KA_TAG, "wait for connection ready");
		if (timeout == 0)
			conn_ready_.wait(locker);
		else
			conn_ready_.wait_for(locker, duration<int>(timeout));
	}
	Log::d(KA_TAG, "connection ready or timeout, %p", connection_.get());
	return connection_;
}

void WSKeepAlive::run() {
	while (work_) {
		handle_shutdown_actions();

		do_connect();

		do_ping();

		rest();
	}
}

void WSKeepAlive::handle_shutdown_actions() {
	lock_guard<mutex> locker(mutex_);
	uint32_t id;
	while (!shutdown_queue_.empty()) {
		id = shutdown_queue_.front();
		shutdown_queue_.pop_front();
		if (conn_equals(id)) {
			Log::d(KA_TAG, "close connection %u",
					connection_->get_id());
			shutdown_queue_.clear();
			connection_->close();
			connection_.reset();
		}
	}
}

void WSKeepAlive::do_connect() {
	if (connection_.get())
		return;
	SpeechConnection* conn = new SpeechConnection(SOCKET_BUF_SIZE);
	Log::d(KA_TAG, "try connect to service %s", service_.c_str());
	if (conn->connect(config_, service_.c_str(), SOCKET_TIMEOUT)) {
		lock_guard<mutex> locker(mutex_);
		connection_.reset(conn);
		conn_ready_.notify_one();
	}
}

void WSKeepAlive::do_ping() {
	if (connection_.get()) {
		if (!connection_->ping(SOCKET_TIMEOUT)) {
			Log::d(KA_TAG, "ping failed, shutdown");
			shutdown(connection_.get());
		} else
			Log::d(KA_TAG, "ping success");
	}
}

void WSKeepAlive::rest() {
	unique_lock<mutex> locker(mutex_);
	if (shutdown_queue_.empty())
		cond_.wait_for(locker, duration<int, std::milli>(interval_time_));
}

bool WSKeepAlive::conn_equals(uint32_t id) {
	if (!connection_.get())
		return false;
	return connection_->get_id() == id;
}

} // namespace speech
} // namespace rokid
