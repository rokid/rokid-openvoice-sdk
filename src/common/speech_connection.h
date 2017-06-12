#pragma once

#include <assert.h>
#include <string>
#include <list>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "log.h"
#include "speech_config.h"
#include "Poco/Net/WebSocket.h"

#define CONN_TAG "speech.Connection"

namespace rokid {
namespace speech {

enum BinRespType {
	BIN_RESP_DATA = 0,
	BIN_RESP_ERROR
};

typedef struct {
	uint32_t length;
	uint32_t type;
	char data[];
} SpeechBinaryResp;

enum ConnectStage {
	// not connected
	CONN_INIT = 0,
	// connected, not auth
	CONN_UNAUTH,
	// connected, sent auth req, wait auth resp
	CONN_WAIT_AUTH,
	// connected and authorized
	CONN_READY,
	// SpeechConnection.release invoked, quit poll thread
	CONN_RELEASED
};

enum ConnectionOpResult {
	CO_SUCCESS = 0,
	CO_INVALID_PB_OBJ,
	CO_CONNECTION_NOT_AVAILABLE,
	CO_SOCKET_ERROR,
	CO_NOT_READY,
	CO_INVALID_PB_DATA,
	CO_CONNECTION_BROKEN,
};

class SpeechConnection {
public:
	SpeechConnection();

	~SpeechConnection();

	// params: 'ws_buf_size'  buf size for websocket frame recv
	//         'svc' can be one of 'tts', 'asr', 'speech'
	void initialize(uint32_t ws_buf_size, SpeechConfig* config,
			const char* svc);

	void release();

	// params: 'timeout' milliseconds
	template <typename PBT>
	int32_t send(PBT& pbitem, uint32_t timeout = 0) {
		std::string buf;
		std::unique_lock<std::mutex> locker(req_mutex_);
		if (!pbitem.SerializeToString(&buf)) {
			Log::w(CONN_TAG, "send: protobuf serialize failed");
			return CO_INVALID_PB_OBJ;
		}
		if (!ensure_connection_available(locker, timeout)) {
			Log::d(CONN_TAG, "send: connection not available");
			return CO_CONNECTION_NOT_AVAILABLE;
		}
		return send(buf.data(), buf.length()) ? CO_SUCCESS
			: CO_SOCKET_ERROR;
	}

	template <typename PBT>
	int32_t recv(PBT& res) {
		SpeechBinaryResp* resp_data;
		std::unique_lock<std::mutex> locker(resp_mutex_);
		while (true) {
			if (!initialized_)
				return CO_NOT_READY;
			if (responses_.empty()) {
				Log::d(CONN_TAG, "recv: wait");
				resp_cond_.wait(locker);
			} else {
				resp_data = responses_.front();
				assert(resp_data);
				responses_.pop_front();
				if (resp_data->type == BIN_RESP_DATA) {
#ifdef SPEECH_SDK_DETAIL_TRACE
					Log::d(CONN_TAG, "recv: get %u bytes from list", resp_data->length);
#endif
					bool r = res.ParseFromArray(resp_data->data, resp_data->length);
					delete resp_data;
					if (!r) {
						Log::w(CONN_TAG, "recv: protobuf parse failed");
						return CO_INVALID_PB_DATA;
					}
				} else {
					Log::d(CONN_TAG, "recv: failed, connection broken");
					delete resp_data;
					return CO_CONNECTION_BROKEN;
				}
				break;
			}
		}
		return CO_SUCCESS;
	}

private:
	void run();

	bool ensure_connection_available(std::unique_lock<std::mutex> &locker,
			uint32_t timeout);

	bool send(const void* data, uint32_t length);

	std::shared_ptr<Poco::Net::WebSocket> connect();

	bool auth();

	bool do_socket_poll();

	void ping();

	void push_error_resp();

	static std::string timestamp();

	static std::string generate_sign(const char* key, const char* devtype,
			const char* devid, const char* svc, const char* version,
			const char* ts, const char* secret);

	static bool init_ssl(SpeechConfig* config);

private:
	std::mutex req_mutex_;
	std::mutex resp_mutex_;
	std::condition_variable req_cond_;
	std::condition_variable resp_cond_;
	std::list<SpeechBinaryResp*> responses_;
	std::shared_ptr<Poco::Net::WebSocket> web_socket_;
	std::thread* thread_;
	SpeechConfig* config_;
	std::string service_type_;
	char* buffer_;
	uint32_t buffer_size_;
	ConnectStage stage_;
	bool initialized_;
	uint32_t pending_ping_;
	static bool ssl_initialized_;
};

} // namespace speech
} // namespace rokid
