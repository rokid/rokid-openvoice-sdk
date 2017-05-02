#pragma once

#include <memory>
#include <string>
#include <exception>
#include <mutex>
#include <chrono>
#include "speech_config.h"
#include "Poco/Timespan.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Net/NetException.h"
#include "log.h"

#define CONN_TAG "speech.Connection"

namespace rokid {
namespace speech {

class SpeechConnection {
public:
	SpeechConnection(uint32_t bufsize);

	~SpeechConnection();

	bool connect(SpeechConfig* config, const char* svc,
			uint32_t timeout = 0);

	void close();

	template <typename PBT>
	bool send(const PBT& pbitem, uint32_t timeout = 0) {
		std::lock_guard<std::mutex> locker(mutex_);
		std::string buf;
		if (!pbitem.SerializeToString(&buf)) {
			Log::w(CONN_TAG, "protobuf serialize failed");
			return false;
		}
		return sendFrame(buf.data(), buf.length(), timeout);
	}

	template <typename PBT>
	bool recv(PBT& res, uint32_t timeout = 0) {
		int flags;
		int c;
		Poco::Timespan ts(timeout, 0);

		web_socket_->setReceiveTimeout(ts);
		try {
			c = web_socket_->receiveFrame(buffer_, buf_size_, flags);
		} catch (Poco::TimeoutException e) {
			Log::w(CONN_TAG, "recv frame timeout: %s", e.what());
			return false;
		} catch (Poco::Net::NetException e) {
			Log::w(CONN_TAG, "recv frame failed: %s", e.what());
			return false;
		}
		if (!res.ParseFromArray(buffer_, c)) {
			Log::w(CONN_TAG, "protobuf parse failed");
			return false;
		}
		return true;
	}

	bool ping(uint32_t timeout = 0);

	inline uint32_t get_id() { return id_; }

private:
	bool auth(SpeechConfig* config, const char* svc, uint32_t timeout);

	static std::string timestamp();

	static std::string generate_sign(const char* key, const char* devtype,
			const char* devid, const char* svc, const char* version,
			const char* ts, const char* secret);

	bool sendFrame(const void* data, uint32_t length, uint32_t timeout);

	static bool init_ssl(SpeechConfig* config);

private:
	std::shared_ptr<Poco::Net::WebSocket> web_socket_;
	std::mutex mutex_;
	char* buffer_;
	uint32_t buf_size_;
	uint32_t id_;

	static uint32_t next_id_;
	static bool ssl_initialized_;
};

} // namespace speech
} // namespace rokid
