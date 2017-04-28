#include <inttypes.h>
#include <sys/time.h>
#include <fstream>
#include <sstream>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include "speech_connection.h"
#include "speech.pb.h"
#include "md5.h"
#include "log.h"

#define CONN_TAG "speech.Connection"

using std::string;
using std::shared_ptr;
using grpc::Status;
using grpc::ClientContext;
using rokid::open::AuthRequest;
using rokid::open::AuthResponse;
using rokid::open::Speech;

namespace rokid {
namespace speech {

static const char* tag_ = "speech.SpeechConnection";

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

shared_ptr<Speech::Stub> SpeechConnection::connect(SpeechConfig* config, const char* svc) {
	// shared_ptr<grpc::ChannelCredentials> insecure(grpc::InsecureChannelCredentials());
	grpc::SslCredentialsOptions options;
	options.pem_root_certs = get_ssl_roots_pem(config->get("ssl_roots_pem"));
	if (options.pem_root_certs.empty()) {
		Log::w(CONN_TAG, "configuration ssl_roots_pem not found, \
				connect to service %s failed", svc);
		return NULL;
	}
	shared_ptr<grpc::ChannelCredentials> creds = grpc::SslCredentials(options);
	Log::d(CONN_TAG, "server address is %s", config->get("server_address", ""));
	shared_ptr<grpc::Channel> channel = grpc::CreateChannel(config->get("server_address", ""), creds);
	shared_ptr<Speech::Stub> stub(Speech::NewStub(channel));
	if (!auth(stub.get(), config, svc))
		return NULL;
	return stub;
}

bool SpeechConnection::auth(Speech::Stub* stub, SpeechConfig* config, const char* svc) {
	AuthRequest req;
	AuthResponse resp;
	const char* auth_key = config->get("key");
	const char* device_type = config->get("device_type_id");
	const char* device_id = config->get("device_id");
	const char* api_version = config->get("api_version", "1");
	const char* secret = config->get("secret");
	std::string ts = timestamp();
	if (auth_key == NULL || device_type == NULL || device_id == NULL
			|| secret == NULL) {
		Log::w(CONN_TAG, "auth return false: %p, %p, %p, %p", auth_key, device_type, device_id, secret);
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

	ClientContext ctx;
	Status status = stub->auth(&ctx, req, &resp);
	if (status.ok()) {
		Log::d(CONN_TAG, "auth result %d", resp.result());
		if (resp.result() == 0)
			return true;
	}
	Log::w(CONN_TAG, "auth failed, %d, %s",
			status.error_code(), status.error_message().c_str());
	return false;
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

string SpeechConnection::generate_sign(const char* key, const char* devtype,
		const char* devid, const char* svc, const char* version,
		const char* ts, const char* secret) {
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
	uint8_t md5_res[16];
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, sign_src.c_str(), sign_src.length());
	MD5_Final(md5_res, &ctx);
	snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
			md5_res[0], md5_res[1], md5_res[2], md5_res[3],
			md5_res[4], md5_res[5], md5_res[6], md5_res[7],
			md5_res[8], md5_res[9], md5_res[10], md5_res[11],
			md5_res[12], md5_res[13], md5_res[14], md5_res[15]);
	Log::d(CONN_TAG, "md5 src = %s, md5 result = %s", sign_src.c_str(), buf);
	return string(buf);
}

SpeechConnection::SpeechConnection() {
}

} // namespace speech
} // namespace rokid
