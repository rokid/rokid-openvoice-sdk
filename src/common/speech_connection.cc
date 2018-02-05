#include <inttypes.h>
#include <sys/time.h>
#include "openssl/md5.h"
#include "speech_connection.h"
#include "nanopb_encoder.h"
#include "nanopb_decoder.h"
#if defined(__GNU_LIBRARY__) || defined(__GLIBC__)
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#endif

#ifdef SPEECH_STATISTIC
#define MAX_PENDING_TRACE_INFOS 128
#define SEND_TRACE_INFO_INTERVAL 1000
#endif

static const char* api_version_ = "2";

using std::mutex;
using std::recursive_mutex;
using std::unique_lock;
using std::lock_guard;
using std::defer_lock;
using std::chrono::milliseconds;
using std::string;
using std::thread;
using std::chrono::steady_clock;
using std::chrono::system_clock;
using std::chrono::duration_cast;
using uWS::Hub;
using uWS::WebSocket;
using uWS::OpCode;

namespace rokid {
namespace speech {

// milliseconds SpeechConnection::ping_interval_ = milliseconds(30000);
// milliseconds SpeechConnection::no_resp_timeout_ = milliseconds(45000);

SpeechConnection::SpeechConnection() : work_thread_(NULL),
		keepalive_thread_(NULL), ws_(NULL), stage_(ConnectStage::INIT),
		CONN_TAG("speech.Connection") {
	prepare_hub();
}

void SpeechConnection::initialize(int32_t ws_buf_size,
		const PrepareOptions& options, const char* svc) {
	snprintf(CONN_TAG_BUF, sizeof(CONN_TAG_BUF), "rokid.Connection.%s", svc);
	CONN_TAG = CONN_TAG_BUF;
	options_ = options;
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "reconn interval = %u, ping interval = %u, no resp timeout = %u",
			options_.reconn_interval, options_.ping_interval, options_.no_resp_timeout);
#endif
	service_type_ = svc;
	stage_ = ConnectStage::INIT;
	// unique_lock<recursive_mutex> locker(reconn_mutex_);
	// reconn_thread_ = new thread([=] { reconn_run(); });
	work_thread_ = new thread([=] { run(); });
	keepalive_thread_ = new thread([=] { keepalive_run(); });
}

void SpeechConnection::release() {
	if (work_thread_ == NULL)
		return;
	// TODO: ensure stage >= CONNECTING
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "release, notify work thread");
#endif
	reconn_mutex_.lock();
	stage_ = ConnectStage::RELEASED;
	hub_.getDefaultGroup<uWS::CLIENT>().close();
	reconn_cond_.notify_all();
	reconn_mutex_.unlock();

#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "join work thread");
#endif
	work_thread_->join();
	delete work_thread_;
	work_thread_ = NULL;
	keepalive_thread_->join();
	delete keepalive_thread_;
	keepalive_thread_ = NULL;
//	Log::d(CONN_TAG, "work thread exited, join reconn thread");
//	reconn_thread_->join();
//	delete reconn_thread_;
//	reconn_thread_ = NULL;
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "work thread exited");
#endif

	// awake all threads of invoking SpeechConnection::recv
	resp_mutex_.lock();
	resp_cond_.notify_all();
	resp_mutex_.unlock();
}

#ifdef SPEECH_STATISTIC
void SpeechConnection::add_trace_info(const TraceInfo& info) {
	req_mutex_.lock();
	_trace_infos.push_back(info);
	if (_trace_infos.size() > MAX_PENDING_TRACE_INFOS)
		_trace_infos.pop_front();
	req_mutex_.unlock();

	reconn_cond_.notify_one();
}

void SpeechConnection::ping(string* payload) {
	const char* data = NULL;
	size_t len = 0;
	if (payload) {
		data = payload->data();
		len = payload->length();
	}
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "send ping frame, payload %lu bytes", len);
#endif
	ws_send(data, len, OpCode::PING);
	lastest_ping_tp_ = steady_clock::now();
}
#else
void SpeechConnection::ping() {
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "send ping frame");
#endif
	ws_send(NULL, 0, OpCode::PING);
	lastest_ping_tp_ = steady_clock::now();
}
#endif

void SpeechConnection::run() {
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "work thread runing");
#endif

	unique_lock<recursive_mutex> locker(reconn_mutex_, defer_lock);
	steady_clock::time_point now;

	while (stage_ != ConnectStage::RELEASED) {
		switch (stage_) {
			case ConnectStage::INIT:
				now = steady_clock::now();
				if (now >= reconn_timepoint_) {
#ifdef SPEECH_SDK_DETAIL_TRACE
					Log::d(CONN_TAG, "work thread: connecting");
#endif
					stage_ = ConnectStage::CONNECTING;
					connect();
				} else {
#ifdef SPEECH_SDK_DETAIL_TRACE
					Log::d(CONN_TAG, "work thread: wait to reconn timepoint");
#endif
					locker.lock();
					if (stage_ == ConnectStage::INIT)
						reconn_cond_.wait_for(locker, reconn_timepoint_ - now);
					locker.unlock();
				}
				break;
			default:
#ifdef SPEECH_SDK_DETAIL_TRACE
				Log::d(CONN_TAG, "work thread: loop");
#endif
				// wait for:
				//   connection lose
				//   socket error
				//   SpeechConnection release
				hub_.run();
#ifdef SPEECH_SDK_DETAIL_TRACE
				Log::d(CONN_TAG, "work thread: loop end");
#endif
		}
	}

#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "work thread quit");
#endif
}

void SpeechConnection::keepalive_run() {
	steady_clock::time_point now;
	unique_lock<recursive_mutex> locker(reconn_mutex_, defer_lock);
	milliseconds timeout;
	milliseconds ping_interval = milliseconds(options_.ping_interval);
	milliseconds no_resp_timeout = milliseconds(options_.no_resp_timeout);
#ifdef SPEECH_STATISTIC
	bool has_trace_info;
#endif

	while (stage_ != ConnectStage::RELEASED) {
		if (stage_ == ConnectStage::READY) {
			now = steady_clock::now();
#ifdef SPEECH_STATISTIC
			has_trace_info = send_trace_info();
#endif
			if (now - lastest_ping_tp_ >= ping_interval) {
				ping();
			}
			if (now - lastest_recv_tp_ >= no_resp_timeout) {
				Log::w(CONN_TAG, "server may no response, try reconnect");
				lastest_recv_tp_ = steady_clock::now();
				ws_->terminate();
			}
			auto d1 = ping_interval - (now - lastest_ping_tp_);
			auto d2 = no_resp_timeout - (now - lastest_recv_tp_);
			timeout = duration_cast<milliseconds>(d1 < d2 ? d1 : d2);
			if (timeout.count() < 0)
				timeout = milliseconds(0);
#ifdef SPEECH_STATISTIC
			if (has_trace_info && timeout.count() > SEND_TRACE_INFO_INTERVAL)
				timeout = milliseconds(SEND_TRACE_INFO_INTERVAL);
#endif
		} else {
			timeout = ping_interval;
		}
		locker.lock();
		reconn_cond_.wait_for(locker, timeout);
		locker.unlock();
	}
}

#ifdef SPEECH_STATISTIC
bool SpeechConnection::send_trace_info() {
	TraceInfo info;
	req_mutex_.lock();
	if (_trace_infos.size() > 0) {
		info = _trace_infos.front();
		_trace_infos.pop_front();
		req_mutex_.unlock();

		PingPayload pp;
		string buf;
		system_clock::time_point tp = system_clock::now();
		int64_t ms = duration_cast<milliseconds>(tp.time_since_epoch()).count();
		pp.set_req_id(info.id);
		pp.set_now_tp(ms);
		ms = duration_cast<milliseconds>(info.req_tp.time_since_epoch()).count();
		pp.set_req_tp(ms);
		ms = duration_cast<milliseconds>(info.resp_tp.time_since_epoch()).count();
		pp.set_resp_tp(ms);
		if (!pp.SerializeToString(&buf)) {
			Log::e(CONN_TAG, "PingPayload serialize failed.");
			return true;
		}
#ifdef SPEECH_SDK_DETAIL_TRACE
		Log::d(CONN_TAG, "send trace info, id = %d", info.id);
#endif
		ping(&buf);
		return true;
	} else
		req_mutex_.unlock();
	return false;
}
#endif

void SpeechConnection::prepare_hub() {
	uWS::Group<uWS::CLIENT>* group = static_cast<uWS::Group<uWS::CLIENT>*>(&hub_);
	group->onConnection([=](WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req) {
			onConnection(ws);
		});
	group->onDisconnection([=](WebSocket<uWS::CLIENT> *ws, int code,
				char* message, size_t length) {
			onDisconnection(ws, code, message, length);
		});
	group->onMessage([=](WebSocket<uWS::CLIENT> *ws, char* message,
				size_t length, OpCode opcode) {
			onMessage(ws, message, length, opcode);
		});
	group->onError([=](void* userdata) { onError(userdata); });
	group->onPong([=](WebSocket<uWS::CLIENT> *ws, char* message, size_t length) {
			onPong(ws, message, length);
		});
}

void SpeechConnection::connect() {
	string uri = get_server_uri();
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "server uri is %s", uri.c_str());
#endif
// glibc bug: gethostbyname not reread resolv.conf if the file changed
// glibc 2.26 fix the bug
#if defined(__GLIBC__)
	if (__GLIBC__ <= 2 && __GLIBC_MINOR__ <= 26)
		res_init();
// old version glibc not defined __GLIBC__, but defined __GNU_LIBRARY__ and __GNU_LIBRARY_MINOR__
#elif defined(__GNU_LIBRARY__)
	res_init();
#endif
	hub_.connect(uri);
}

string SpeechConnection::get_server_uri() {
	char tmp[64];
	snprintf(tmp, sizeof(tmp), "wss://%s:%u%s",
			options_.host.c_str(), options_.port, options_.branch.c_str());
	return string(tmp);
}

void SpeechConnection::onConnection(WebSocket<uWS::CLIENT> *ws) {
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "uws connected, %p", ws);
#endif
	stage_ = ConnectStage::UNAUTH;
	auth(ws);
}

void SpeechConnection::onDisconnection(uWS::WebSocket<uWS::CLIENT> *ws,
		int code, char* message, size_t length) {
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "uws disconnected, code %d, msg length %d, stage %d",
			code, length, static_cast<int>(stage_));
#endif
	ws_ = NULL;
	if (stage_ != ConnectStage::RELEASED) {
		stage_ = ConnectStage::INIT;
		push_error_resp();
	}
}

void SpeechConnection::onMessage(uWS::WebSocket<uWS::CLIENT> *ws,
		char* message, size_t length, uWS::OpCode opcode) {
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "uws recv message, length %d, opcode 0x%x", length, opcode);
#endif
	lastest_recv_tp_ = steady_clock::now();
	switch (stage_) {
		case ConnectStage::UNAUTH:
			handle_auth_result(ws, message, length, opcode);
			break;
		case ConnectStage::READY:
			push_resp_data(message, length);
			break;
		default:
			Log::w(CONN_TAG, "recv %d bytes, opcode %d, but connect stage is %d",
					length, opcode, static_cast<int>(stage_));
	}
}

void SpeechConnection::onError(void* userdata) {
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "uws error: userdata %p, stage %d", userdata, static_cast<int>(stage_));
#endif
	if (ws_) {
		ws_->close();
		ws_ = NULL;
	}
	if (stage_ != ConnectStage::RELEASED) {
		stage_ = ConnectStage::INIT;
		reconn_timepoint_ = steady_clock::now() + milliseconds(options_.reconn_interval);
	}
}

void SpeechConnection::onPong(uWS::WebSocket<uWS::CLIENT> *ws,
		char* message, size_t length) {
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "uws recv pong: stage %d", static_cast<int>(stage_));
#endif
	lastest_recv_tp_ = steady_clock::now();
}

bool SpeechConnection::auth(WebSocket<uWS::CLIENT> *ws) {
	AuthRequest req;
	const char* svc = service_type_.c_str();
	string ts = timestamp();
	if (options_.key.empty()
			|| options_.device_type_id.empty()
			|| options_.device_id.empty()
			|| options_.secret.empty()) {
		Log::w(CONN_TAG, "auth invalid param");
		return false;
	}

	req.set_key(options_.key);
	req.set_device_type_id(options_.device_type_id);
	req.set_device_id(options_.device_id);
	req.set_service(svc);
	req.set_version(api_version_);
	req.set_timestamp(ts);
	req.set_sign(generate_sign(options_.key.c_str(),
					options_.device_type_id.c_str(),
					options_.device_id.c_str(), svc,
					api_version_, ts.c_str(),
					options_.secret.c_str()));

	std::string buf;
	if (!req.SerializeToString(&buf)) {
		Log::w(CONN_TAG, "auth: protobuf serialize failed");
		return false;
	}
	ws->send(buf.data(), buf.length(), OpCode::BINARY);
	return true;
}

string SpeechConnection::timestamp() {
	struct timeval tv;
	uint64_t usecs;
	gettimeofday(&tv, NULL);
	usecs = tv.tv_sec;
	usecs *= 1000000;
	usecs += tv.tv_usec;
	char buf[64];
	snprintf(buf, sizeof(buf), "%" PRIu64, usecs);
	return string(buf);
}

string SpeechConnection::generate_sign(const char* key,
		const char* devtype, const char* devid, const char* svc,
		const char* version, const char* ts, const char* secret) {
	string sign_src;
	char buf[64];
	snprintf(buf, sizeof(buf), "key=%s", key);
	sign_src.append(buf);
	snprintf(buf, sizeof(buf), "&device_type_id=%s", devtype);
	sign_src.append(buf);
	snprintf(buf, sizeof(buf), "&device_id=%s", devid);
	sign_src.append(buf);
	snprintf(buf, sizeof(buf), "&service=%s", svc);
	sign_src.append(buf);
	snprintf(buf, sizeof(buf), "&version=%s", version);
	sign_src.append(buf);
	snprintf(buf, sizeof(buf), "&time=%s", ts);
	sign_src.append(buf);
	snprintf(buf, sizeof(buf), "&secret=%s", secret);
	sign_src.append(buf);
	uint8_t md5_res[MD5_DIGEST_LENGTH];
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, sign_src.c_str(), sign_src.length());
	MD5_Final(md5_res, &ctx);
	snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
			md5_res[0], md5_res[1], md5_res[2], md5_res[3],
			md5_res[4], md5_res[5], md5_res[6], md5_res[7],
			md5_res[8], md5_res[9], md5_res[10], md5_res[11],
			md5_res[12], md5_res[13], md5_res[14], md5_res[15]);
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "md5 src = %s, md5 result = %s",
			sign_src.c_str(), buf);
#endif
	return string(buf);
}

void SpeechConnection::handle_auth_result(uWS::WebSocket<uWS::CLIENT> *ws,
		char* message, size_t length, uWS::OpCode opcode) {
	AuthResponse resp;
	if (!resp.ParseFromArray(message, length)) {
		Log::w(CONN_TAG, "auth response parse failed, not correct protobuf");
		return;
	}
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "auth result = %d", resp.result());
#endif
	if (resp.result() == 0) {
		req_mutex_.lock();
		stage_ = ConnectStage::READY;
		ws_ = ws;
		// workaround for Hub loop nerver return when server no response
		// set a timer, epoll_wait will awake periodic
		hub_.getDefaultGroup<uWS::CLIENT>().setTimer(4000);
		req_cond_.notify_one();
		req_mutex_.unlock();
	} else {
		reconn_timepoint_ = steady_clock::now() + milliseconds(options_.reconn_interval);
		ws->close();
	}
}

bool SpeechConnection::ensure_connection_available(
		unique_lock<mutex>& locker, uint32_t timeout) {
	if (stage_ != ConnectStage::READY) {
#ifdef SPEECH_SDK_DETAIL_TRACE
		Log::d(CONN_TAG, "stage is %d, wait connection available, timeout %u ms", stage_, timeout);
#endif
		if (timeout == 0)
			req_cond_.wait(locker);
		else {
			milliseconds ms(timeout);
			req_cond_.wait_for(locker, ms);
		}
	}
	return stage_ == ConnectStage::READY;
}

void SpeechConnection::push_error_resp() {
	lock_guard<mutex> locker(resp_mutex_);
	SpeechBinaryResp* bin_resp;
	if (stage_ == ConnectStage::READY) {
#ifdef SPEECH_SDK_DETAIL_TRACE
		Log::d(CONN_TAG, "push error response to list");
#endif
		bin_resp = (SpeechBinaryResp*)malloc(sizeof(SpeechBinaryResp));
		bin_resp->length = 0;
		bin_resp->type = BinRespType::ERROR;
		responses_.push_back(bin_resp);
		resp_cond_.notify_one();
	}
}

void SpeechConnection::push_resp_data(char* msg, size_t length) {
	SpeechBinaryResp* bin_resp;
	bin_resp = (SpeechBinaryResp*)malloc(length + sizeof(SpeechBinaryResp));
	bin_resp->length = length;
	bin_resp->type = BinRespType::DATA;
	memcpy(bin_resp->data, msg, length);
	unique_lock<mutex> locker(resp_mutex_);
	responses_.push_back(bin_resp);
	resp_cond_.notify_one();
}

void SpeechConnection::ws_send(const char* msg, size_t length, uWS::OpCode op) {
	if (ws_) {
#ifdef SPEECH_SDK_DETAIL_TRACE
		Log::d(CONN_TAG, "SpeechConnection.ws_send: %lu bytes", length);
#endif
		ws_->send(msg, length, op);
	}
}

PrepareOptions::PrepareOptions() {
	host = "localhost";
	port = 80;
	branch = "/";
	reconn_interval = 20000;
	ping_interval = 30000;
	no_resp_timeout = 45000;
}

PrepareOptions& PrepareOptions::operator = (const PrepareOptions& options) {
	this->host = options.host;
	this->port = options.port;
	this->branch = options.branch;
	this->key = options.key;
	this->device_type_id = options.device_type_id;
	this->secret = options.secret;
	this->device_id = options.device_id;
	this->reconn_interval = options.reconn_interval;
	this->ping_interval = options.ping_interval;
	this->no_resp_timeout = options.no_resp_timeout;
	return *this;
}



} // namespace speech
} // namespace rokid
