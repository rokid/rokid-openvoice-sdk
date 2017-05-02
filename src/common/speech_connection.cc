#include <inttypes.h>
#include <sys/time.h>
#include <stdlib.h>
#include "openssl/md5.h"
#include "speech_connection.h"
#include "speech.pb.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPMessage.h"
#include "Poco/SharedPtr.h"
#include "Poco/Net/PrivateKeyPassphraseHandler.h"
#include "Poco/Net/InvalidCertificateHandler.h"
#include "Poco/Net/ConsoleCertificateHandler.h"
#include "Poco/Net/KeyConsoleHandler.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/Context.h"

#define MIN_BUF_SIZE 4096

using std::string;
using std::shared_ptr;
using std::exception;
using rokid::open::AuthRequest;
using rokid::open::AuthResponse;
using Poco::Timespan;
using Poco::Net::HTTPSClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::WebSocket;
using Poco::SharedPtr;
using Poco::Net::PrivateKeyPassphraseHandler;
using Poco::Net::KeyConsoleHandler;
using Poco::Net::InvalidCertificateHandler;
using Poco::Net::ConsoleCertificateHandler;
using Poco::Net::SSLManager;
using Poco::Net::Context;
using Poco::Net::HTTPClientSession;

namespace rokid {
namespace speech {

uint32_t SpeechConnection::next_id_ = 0;
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

SpeechConnection::SpeechConnection(uint32_t bufsize) {
	if (bufsize < MIN_BUF_SIZE)
		bufsize = MIN_BUF_SIZE;
	buffer_ = new char[bufsize];
	buf_size_ = bufsize;
	id_ = ++next_id_;
}

SpeechConnection::~SpeechConnection() {
	delete buffer_;
}

bool SpeechConnection::connect(SpeechConfig* config, const char* svc,
		uint32_t timeout) {
	if (!init_ssl(config)) {
		Log::e(CONN_TAG, "connect: init ssl failed");
		return false;
	}

	const char* host = config->get("host", "localhost");
	const char* port_str = config->get("port", "80");
	const char* branch = config->get("branch", "/");
	int port = atoi(port_str);
	HTTPSClientSession cs(host, port);
	HTTPRequest request(HTTPRequest::HTTP_GET, branch,
			HTTPMessage::HTTP_1_1);
	HTTPResponse response;

	Log::d(CONN_TAG, "server address is %s:%d%s", host, port, branch);
	// request.set("origin", host);

	try {
		web_socket_.reset(new WebSocket(cs, request, response));
	} catch (exception e) {
		Log::w(CONN_TAG, "websocket connect failed: %s", e.what());
		web_socket_.reset();
		return false;
	}
	if (!auth(config, svc, timeout)) {
		web_socket_.reset();
		return false;
	}
	return true;
}

void SpeechConnection::close() {
	web_socket_->close();
	web_socket_.reset();
}

bool SpeechConnection::ping(uint32_t timeout) {
	// TODO: send ping frame
	return true;
}

bool SpeechConnection::auth(SpeechConfig* config, const char* svc,
		uint32_t timeout) {
	AuthRequest req;
	AuthResponse resp;
	const char* auth_key = config->get("key");
	const char* device_type = config->get("device_type_id");
	const char* device_id = config->get("device_id");
	const char* api_version = config->get("api_version", "1");
	const char* secret = config->get("secret");
	string ts = timestamp();
	if (auth_key == NULL || device_type == NULL || device_id == NULL
			|| secret == NULL) {
		Log::w(CONN_TAG, "auth return false: %p, %p, %p, %p",
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

	if (!this->send(req, timeout))
		return false;
	if (!this->recv(resp, timeout))
		return false;
	Log::d(CONN_TAG, "auth result is %d", resp.result());
	return resp.result() == 0;
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

bool SpeechConnection::sendFrame(const void* data, uint32_t length,
		uint32_t timeout) {
	int c;
	uint32_t offset = 0;
	Timespan wstimeout(timeout, 0);

	web_socket_->setSendTimeout(wstimeout);
	try {
		while (offset < length) {
			c = web_socket_->sendFrame(reinterpret_cast<const char*>(data) + offset,
					length - offset, WebSocket::FRAME_BINARY);
			if (c < 0) {
				Log::w(CONN_TAG, "socket send return negative value %d, \
						what happen?", c);
				return false;
			}
			offset += c;
			Log::d(CONN_TAG, "websocket send frame %u:%lu bytes",
					offset, length);
		}
	} catch (exception e) {
		Log::w(CONN_TAG, "send frame failed: %s", e.what());
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
		} catch (std::exception e) {
			Log::e(CONN_TAG, "initialize ssl failed: %s", e.what());
			return false;
		}
		ssl_initialized_ = true;
	}
	return true;
}

} // namespace speech
} // namespace rokid
