#include <inttypes.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include "openssl/md5.h"
#include "speech_connection.h"
#include "speech.pb.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPMessage.h"
#include "Poco/Exception.h"
#include "Poco/SharedPtr.h"
#include "Poco/Net/PrivateKeyPassphraseHandler.h"
#include "Poco/Net/InvalidCertificateHandler.h"
#include "Poco/Net/ConsoleCertificateHandler.h"
#include "Poco/Net/KeyConsoleHandler.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/Context.h"

#define MIN_BUF_SIZE 4096
#define CONNECT_RETRY_TIMEOUT 30000
#define KEEPALIVE_TIMEOUT 20000
#define SOCKET_POLL_TIMEOUT 1000
#define AUTH_RESP_TIMEOUT 10000

using std::string;
using std::shared_ptr;
using std::exception;
using std::lock_guard;
using std::unique_lock;
using std::thread;
using std::mutex;
using std::chrono::duration;
using rokid::open::AuthRequest;
using rokid::open::AuthResponse;
using Poco::Timespan;
using Poco::Net::HTTPSClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::WebSocket;
using Poco::Exception;
using Poco::SharedPtr;
using Poco::Net::PrivateKeyPassphraseHandler;
using Poco::Net::KeyConsoleHandler;
using Poco::Net::InvalidCertificateHandler;
using Poco::Net::ConsoleCertificateHandler;
using Poco::Net::SSLManager;
using Poco::Net::Context;
using Poco::Net::HTTPClientSession;
using Poco::Net::Socket;

namespace rokid {
namespace speech {

bool SpeechConnection::ssl_initialized_ = false;

/** comment temporary
static std::string get_ssl_roots_pem(const char* pem_file) {
	Log::d(CONN_TAG, "ssl_roots_pem is %s", pem_file);
	if (pem_file == NULL)
		return "";
	std::ifstream stream(pem_file);
	std::stringstream ss;
	ss << stream.rdbuf();
	stream.close();
	return ss.str();
}
*/

SpeechConnection::SpeechConnection() : initialized_(false) {
}

SpeechConnection::~SpeechConnection() {
	release();
}

void SpeechConnection::initialize(uint32_t ws_buf_size,
		SpeechConfig* config, const char* svc) {
	if (ws_buf_size < MIN_BUF_SIZE)
		ws_buf_size = MIN_BUF_SIZE;
	buffer_size_ = ws_buf_size;
	config_ = config;
	service_type_ = svc;
	stage_ = CONN_INIT;
	pending_ping_ = 0;
	unique_lock<mutex> locker(resp_mutex_);
	thread_ = new thread([=] { run(); });
	// wait thread run
	resp_cond_.wait(locker);
}

void SpeechConnection::release() {
	if (!initialized_)
		return;
	unique_lock<mutex> locker(resp_mutex_);
	initialized_ = false;
	resp_cond_.notify_one();
	locker.unlock();

	thread_->join();
	delete thread_;
	delete buffer_;

	unique_lock<mutex> rlocker(req_mutex_);
	req_cond_.notify_all();
}

void SpeechConnection::run() {
	buffer_ = new char[buffer_size_];
	unique_lock<mutex> locker(resp_mutex_);
	initialized_ = true;
	// notify initialize(), thread already run
	resp_cond_.notify_one();
	locker.unlock();

	while (true) {
		locker.lock();
		if (!initialized_) {
			Log::i(CONN_TAG, "released, quit run()");
			break;
		}
		locker.unlock();

		if (stage_ == CONN_INIT) {
			web_socket_ = connect();
			if (web_socket_.get()) {
				Log::i(CONN_TAG, "connect to server success, do auth");
				stage_ = CONN_UNAUTH;
				continue;
			} else
				Log::i(CONN_TAG, "connect to server failed, wait a "
						"while and retry");
		} else if (stage_ == CONN_UNAUTH) {
			if (auth()) {
				Log::d(CONN_TAG, "auth req success, wait auth result");
				lock_guard<mutex> locker(req_mutex_);
				stage_ = CONN_WAIT_AUTH;
				continue;
			} else {
				Log::d(CONN_TAG, "auth req failed, wait and reconnect");
				web_socket_->close();
				web_socket_.reset();
				stage_ = CONN_INIT;
			}
		} else {
			if (do_socket_poll())
				continue;
		}

		usleep(CONNECT_RETRY_TIMEOUT * 1000);
	}
	stage_ = CONN_RELEASED;
}

shared_ptr<WebSocket> SpeechConnection::connect() {
	HTTPClientSession* cs;
	const char* host = config_->get("host", "localhost");
	const char* port_str = config_->get("port", "80");
	const char* branch = config_->get("branch", "/");
	int port = atoi(port_str);

	const char* ssl_roots_pem = config_->get("ssl_roots_pem", NULL);
	if (ssl_roots_pem == NULL) {
		cs = new HTTPClientSession(host, port);
	} else {
		if (!init_ssl(config_)) {
			Log::e(CONN_TAG, "connect: init ssl failed");
			return NULL;
		}
		cs = new HTTPSClientSession(host, port);
	}
	HTTPRequest request(HTTPRequest::HTTP_GET, branch,
			HTTPMessage::HTTP_1_1);
	HTTPResponse response;
	shared_ptr<WebSocket> sock;

	Log::d(CONN_TAG, "server address is %s:%d%s, use ssl %d",
			host, port, branch, ssl_initialized_);

	try {
		sock.reset(new WebSocket(*cs, request, response));
	} catch (Exception e) {
		Log::w(CONN_TAG, "websocket connect failed: %s",
				e.displayText().c_str());
		delete cs;
		return NULL;
	}
	delete cs;
	return sock;
}

bool SpeechConnection::auth() {
	AuthRequest req;
	AuthResponse resp;
	const char* auth_key = config_->get("key");
	const char* device_type = config_->get("device_type_id");
	const char* device_id = config_->get("device_id");
	const char* api_version = config_->get("api_version", "1");
	const char* secret = config_->get("secret");
	const char* svc = service_type_.c_str();
	string ts = timestamp();
	if (auth_key == NULL || device_type == NULL || device_id == NULL
			|| secret == NULL) {
		Log::w(CONN_TAG, "auth invalid param: %p, %p, %p, %p",
				auth_key, device_type, device_id, secret);
		return false;
	}

	req.set_key(auth_key);
	req.set_device_type_id(device_type);
	req.set_device_id(device_id);
	req.set_service(svc);
	req.set_version(api_version);
	req.set_timestamp(ts);
	req.set_sign(generate_sign(auth_key,
					device_type, device_id, svc,
					api_version, ts.c_str(),
					secret));

	assert(web_socket_.get());
	std::string buf;
	if (!req.SerializeToString(&buf)) {
		Log::w(CONN_TAG, "auth: protobuf serialize failed");
		return false;
	}
	std::lock_guard<std::mutex> locker(req_mutex_);
	return this->send((const void*)buf.data(), buf.length());
}

// return: true  immediate try reconnect
//         false wait a while and try reconnect
bool SpeechConnection::do_socket_poll() {
	Timespan timeout(SOCKET_POLL_TIMEOUT / 1000, 0);
	int flags;
	int c;
	SpeechBinaryResp* bin_resp;
	bool reconn = true;
	int32_t keepalive_timeout = KEEPALIVE_TIMEOUT;

	while (true) {
		if (web_socket_->available() <= 0) {
			if (!web_socket_->poll(timeout, WebSocket::SELECT_READ
						| WebSocket::SELECT_ERROR)) {
				if (!initialized_) {
					Log::d(CONN_TAG, "connection released, quit do_socket_poll *a*");
					break;
				}
				keepalive_timeout -= SOCKET_POLL_TIMEOUT;
				if (keepalive_timeout <= 0) {
#ifdef SPEECH_SDK_DETAIL_TRACE
					Log::d(CONN_TAG, "it's time to ws keepalive");
#endif
					// timeout
					if (stage_ == CONN_WAIT_AUTH) {
						Log::i(CONN_TAG, "wait auth result timeout, try reconnect");
						goto close_conn;
					}
					if (pending_ping_ > 0) {
						Log::w(CONN_TAG, "previous ping not received pong, "
								"connection may broken, try reconnect");
						pending_ping_ = 0;
						goto close_conn;
					}
					ping();
					keepalive_timeout = KEEPALIVE_TIMEOUT;
				}
				continue;
			}
		}
		if (!initialized_) {
			Log::d(CONN_TAG, "connection released, quit do_socket_poll *b*");
			break;
		}
		keepalive_timeout = KEEPALIVE_TIMEOUT;

		try {
			c = web_socket_->receiveFrame(buffer_, buffer_size_, flags);
#ifdef SPEECH_SDK_DETAIL_TRACE
			Log::d(CONN_TAG, "socket recv %d bytes, flags 0x%x", c, flags);
			Log::d(CONN_TAG, "after recv frame, avail = %d", web_socket_->available());
#endif
		} catch (Exception e) {
			Log::w(CONN_TAG, "websocket receive failed, exception = %s",
					e.displayText().c_str());
			push_error_resp();
			goto close_conn;
		}
		if (c == 0) {
			if ((flags & WebSocket::FRAME_OP_BITMASK) ==
					WebSocket::FRAME_OP_PONG) {
#ifdef SPEECH_SDK_DETAIL_TRACE
				Log::d(CONN_TAG, "recv pong frame");
#endif
				--pending_ping_;
			} else {
				push_error_resp();
				goto close_conn;
			}
		} else if (c < 0) {
			push_error_resp();
			goto close_conn;
		} else {
			bin_resp = (SpeechBinaryResp*)malloc(c + sizeof(SpeechBinaryResp));
			bin_resp->length = c;
			bin_resp->type = BIN_RESP_DATA;
			memcpy(bin_resp->data, buffer_, c);
			unique_lock<mutex> locker(resp_mutex_);
			responses_.push_back(bin_resp);
			if (stage_ == CONN_WAIT_AUTH) {
				locker.unlock();
				AuthResponse auth_res;
				ConnectionOpResult opr;
				opr = this->recv(auth_res, AUTH_RESP_TIMEOUT);
				if (opr != ConnectionOpResult::SUCCESS) {
					Log::w(CONN_TAG, "auth result recv failed %d, "
							"try reconnect", opr);
					reconn = false;
					goto close_conn;
				}
				Log::d(CONN_TAG, "auth result = %d", auth_res.result());
				if (auth_res.result() != 0) {
					reconn = false;
					goto close_conn;
				}
				lock_guard<mutex> locker(req_mutex_);
				stage_ = CONN_READY;
				req_cond_.notify_all();
			} else {
#ifdef SPEECH_SDK_DETAIL_TRACE
				Log::d(CONN_TAG, "recv resp, add to list");
#endif
				resp_cond_.notify_one();
			}
		}
	}

close_conn:
	Log::d(CONN_TAG, "close websocket, reconnect immediate ? %d", reconn);
	unique_lock<mutex> req_locker(req_mutex_);
	stage_ = CONN_INIT;
	web_socket_->close();
	web_socket_.reset();
	req_locker.unlock();

	// awake 'recv', prevent block forever if server no response
	lock_guard<mutex> resp_locker(resp_mutex_);
	resp_cond_.notify_one();
	return reconn;
}

void SpeechConnection::push_error_resp() {
	lock_guard<mutex> locker(resp_mutex_);
	SpeechBinaryResp* bin_resp;
	if (stage_ == CONN_READY) {
#ifdef SPEECH_SDK_DETAIL_TRACE
		Log::d(CONN_TAG, "push error response to list");
#endif
		bin_resp = (SpeechBinaryResp*)malloc(sizeof(SpeechBinaryResp));
		bin_resp->length = 0;
		bin_resp->type = BIN_RESP_ERROR;
		responses_.push_back(bin_resp);
	}
}

void SpeechConnection::ping() {
	assert(web_socket_.get());
	++pending_ping_;
#ifdef SPEECH_SDK_DETAIL_TRACE
	Log::d(CONN_TAG, "send ping frame");
#endif
	lock_guard<mutex> locker(req_mutex_);
	web_socket_->sendFrame(NULL, 0, WebSocket::FRAME_FLAG_FIN
			| WebSocket::FRAME_OP_PING);
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
	Log::d(CONN_TAG, "md5 src = %s, md5 result = %s",
			sign_src.c_str(), buf);
	return string(buf);
}

bool SpeechConnection::send(const void* data, uint32_t length) {
	int c;
	uint32_t offset = 0;

	assert(web_socket_.get());
	try {
		while (offset < length) {
			c = web_socket_->sendFrame(reinterpret_cast<const char*>(data) + offset,
					length - offset, WebSocket::FRAME_BINARY);
			if (c <= 0) {
				Log::w(CONN_TAG, "socket send failed, res = %d", c);
				return false;
			}

			offset += c;
#ifdef SPEECH_SDK_DETAIL_TRACE
			Log::d(CONN_TAG, "websocket send frame %u:%lu bytes",
					offset, length);
#endif
		}
	} catch (Exception e) {
		Log::w(CONN_TAG, "send frame failed: %s", e.displayText().c_str());
		return false;
	}
	return true;
}

bool SpeechConnection::init_ssl(SpeechConfig* config) {
	if (!ssl_initialized_) {
		try {
			Poco::Net::initializeSSL();
			SharedPtr<PrivateKeyPassphraseHandler> key_handler
				= new KeyConsoleHandler(false);
			SharedPtr<InvalidCertificateHandler> cert_handler
				= new ConsoleCertificateHandler(false);
			Context::Ptr context = new Context(Context::CLIENT_USE, "",
					"", config->get("ssl_roots_pem", ""));
			SSLManager::instance().initializeClient(key_handler,
					cert_handler, context);
		} catch (Exception e) {
			Log::e(CONN_TAG, "initialize ssl failed: %s", e.displayText().c_str());
			return false;
		}
		ssl_initialized_ = true;
	}
	return true;
}

bool SpeechConnection::ensure_connection_available(
		unique_lock<mutex>& locker, uint32_t timeout) {
	if (stage_ != CONN_READY) {
		if (timeout == 0)
			req_cond_.wait(locker);
		else {
			duration<int, std::milli> ms(timeout);
			req_cond_.wait_for(locker, ms);
		}
	}
	return stage_ == CONN_READY;
}

} // namespace speech
} // namespace rokid
