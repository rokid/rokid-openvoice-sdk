#pragma once

#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "speech_connection.h"

namespace rokid {
namespace speech {

class WSKeepAlive {
public:
	WSKeepAlive();

	// 'interval_time' is milliseconds
	void start(uint32_t interval_time, SpeechConfig* config,
			const char* svc);

	void close();

	void shutdown(SpeechConnection* conn);

	// block util connection available
	// 'timeout' is seconds
	std::shared_ptr<SpeechConnection> get_conn(uint32_t timeout);

private:
	void run();

	void handle_shutdown_actions();

	void do_connect();

	void do_ping();

	void rest();

	bool conn_equals(uint32_t id);

private:
	std::thread* thread_;
	std::shared_ptr<SpeechConnection> connection_;
	std::list<uint32_t> shutdown_queue_;
	std::mutex mutex_;
	std::condition_variable cond_;
	std::condition_variable conn_ready_;
	// milliseconds
	uint32_t interval_time_;
	SpeechConfig* config_;
	std::string service_;
	bool work_;
};

} // namespace speech
} // namespace rokid
