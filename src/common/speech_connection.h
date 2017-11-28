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
#include "speech.h"
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

#ifdef SPEECH_STATISTIC
typedef struct {
	int32_t id;
	std::chrono::system_clock::time_point req_tp;
	std::chrono::system_clock::time_point resp_tp;
} TraceInfo;
#endif

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

enum class ConnectionOpResult {
	SUCCESS = 0,
	INVALID_PB_OBJ,
	CONNECTION_NOT_AVAILABLE,
	SOCKET_ERROR,
	NOT_READY,
	INVALID_PB_DATA,
	CONNECTION_BROKEN,
	TIMEOUT,
};

class SpeechConnection {
public:
	SpeechConnection();

	~SpeechConnection();

	// params: 'ws_buf_size'  buf size for websocket frame recv
	//         'svc' can be one of 'tts', 'speech'
	void initialize(uint32_t ws_buf_size, const PrepareOptions& options,
			const char* svc);

	void release();

	// params: 'timeout' milliseconds
	template <typename PBT>
	ConnectionOpResult send(PBT& pbitem, uint32_t timeout = 0) {
		std::string buf;
		std::unique_lock<std::mutex> locker(req_mutex_);
		if (!pbitem.SerializeToString(&buf)) {
			Log::w(CONN_TAG, "send: protobuf serialize failed");
			return ConnectionOpResult::INVALID_PB_OBJ;
		}
		if (!ensure_connection_available(locker, timeout)) {
			Log::d(CONN_TAG, "send: connection not available");
			return ConnectionOpResult::CONNECTION_NOT_AVAILABLE;
		}
		return send(buf.data(), buf.length())
			? ConnectionOpResult::SUCCESS
			: ConnectionOpResult::SOCKET_ERROR;
	}

	template <typename PBT>
	ConnectionOpResult recv(PBT& res, uint32_t timeout) {
		SpeechBinaryResp* resp_data;
		std::unique_lock<std::mutex> locker(resp_mutex_);

		if (!initialized_)
			return ConnectionOpResult::NOT_READY;
		if (responses_.empty()) {
			std::chrono::duration<int, std::milli> ms(timeout);
			resp_cond_.wait_for(locker, ms);
			if (!initialized_)
				return ConnectionOpResult::NOT_READY;
		}
		if (!responses_.empty()) {
			resp_data = responses_.front();
			assert(resp_data);
			responses_.pop_front();
			if (resp_data->type == BIN_RESP_DATA) {
				bool r = res.ParseFromArray(resp_data->data,
						resp_data->length);
				delete resp_data;
				if (!r) {
					Log::w(CONN_TAG, "recv: protobuf parse failed");
					return ConnectionOpResult::INVALID_PB_DATA;
				}
				return ConnectionOpResult::SUCCESS;
			}
			Log::w(CONN_TAG, "recv: failed, connection broken");
			delete resp_data;
			return ConnectionOpResult::CONNECTION_BROKEN;
		}
		return ConnectionOpResult::TIMEOUT;
	}

#ifdef SPEECH_STATISTIC
	void add_trace_info(const TraceInfo& info);
#endif

private:
	void run();

	bool ensure_connection_available(std::unique_lock<std::mutex> &locker,
			uint32_t timeout);

	bool send(const void* data, uint32_t length);

	std::shared_ptr<Poco::Net::WebSocket> connect();

	bool auth();

	bool do_socket_poll();

#ifdef SPEECH_STATISTIC
	void ping(const TraceInfo* info = NULL);
#else
	void ping();
#endif

	void push_error_resp();

	bool check_keepalive();

	static std::string timestamp();

	static std::string generate_sign(const char* key, const char* devtype,
			const char* devid, const char* svc, const char* version,
			const char* ts, const char* secret);

	static bool init_ssl();

private:
	std::mutex req_mutex_;
	std::mutex resp_mutex_;
	std::condition_variable req_cond_;
	std::condition_variable resp_cond_;
	std::list<SpeechBinaryResp*> responses_;
	std::shared_ptr<Poco::Net::WebSocket> web_socket_;
	std::thread* thread_;
	PrepareOptions options_;
	std::string service_type_;
	char* buffer_;
	uint32_t buffer_size_;
	ConnectStage stage_;
	bool initialized_;
	std::chrono::steady_clock::time_point _lastest_send_tp;
	std::chrono::steady_clock::time_point _lastest_recv_tp;
#ifdef SPEECH_STATISTIC
	std::list<TraceInfo> _trace_infos;
#endif
	static bool ssl_initialized_;
};

} // namespace speech
} // namespace rokid
