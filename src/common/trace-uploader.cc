#include "trace-uploader.h"

#ifdef ROKID_UPLOAD_TRACE

#include <iostream>
#include <chrono>
#include "json.h"
#include "Poco/UUIDGenerator.h"
#include "Poco/URI.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "openssl/md5.h"
#include "rlog.h"

#define API_VERSION "1.0.0"
#define EVENT_SRC 1
#define SERVER_URI "https://das-tc-service-pro.rokid.com/das-tracing-collection/tracingUpload"
#define MAX_KVS 10
#define TAG "speech.trace-uploader"

using namespace std;
using namespace std::chrono;
using namespace Poco;
using namespace Poco::Net;

class JSONGenerator {
public:
	JSONGenerator(const string& dev_id, const string& dev_tp_id);

	void start();

	void add_event(shared_ptr<TraceEvent>& ev);

	void end();

	const string& json_str() const { return json_string; }

private:
	void gen_sign(const string& nonce, string& result);

	void add_event_kvs(struct json_object* jobj, KeyValueArray& kvs);

private:
	string device_id;
	string device_type_id;
	string os_version;
	string master_id;
	int32_t event_source = EVENT_SRC;
	string json_string;
	struct json_object* json_root_obj = nullptr;
	struct json_object* data_array = nullptr;
};

JSONGenerator::JSONGenerator(const string& dev_id, const string& dev_tp_id) {
	device_id = dev_id;
	device_type_id = dev_tp_id;
}

void JSONGenerator::start() {
	json_root_obj = json_object_new_object();

	UUID uuid = UUIDGenerator::defaultGenerator().createRandom();
	struct json_object* subobj = json_object_new_string(uuid.toString().c_str());
	json_object_object_add(json_root_obj, "requestId", subobj);
	json_object_object_add(json_root_obj, "nonce", subobj);
	string sign;
	gen_sign(uuid.toString(), sign);
	subobj = json_object_new_string(sign.c_str());
	json_object_object_add(json_root_obj, "sign", subobj);
	subobj = json_object_new_string("MD5");
	json_object_object_add(json_root_obj, "signMethod", subobj);
	subobj = json_object_new_string(API_VERSION);
	json_object_object_add(json_root_obj, "apiVersion", subobj);
	subobj = json_object_new_int(event_source);
	json_object_object_add(json_root_obj, "eventSrc", subobj);
	subobj = json_object_new_string(device_id.c_str());
	json_object_object_add(json_root_obj, "rokidId", subobj);
	data_array = json_object_new_array();
}

void JSONGenerator::add_event(shared_ptr<TraceEvent>& ev) {
	struct json_object* evobj = json_object_new_object();
	struct json_object* subobj;

	if (os_version.length()) {
		subobj = json_object_new_string(os_version.c_str());
		json_object_object_add(evobj, "osVersion", subobj);
	}
	system_clock::time_point now = system_clock::now();
	milliseconds ts = duration_cast<milliseconds>(now.time_since_epoch());
	subobj = json_object_new_int64(ts.count());
	json_object_object_add(evobj, "timestamp", subobj);
	subobj = json_object_new_int(ev->type);
	json_object_object_add(evobj, "eventType", subobj);
	subobj = json_object_new_string(ev->id.c_str());
	json_object_object_add(evobj, "eventId", subobj);
	subobj = json_object_new_string(ev->name.c_str());
	json_object_object_add(evobj, "eventName", subobj);
	if (master_id.length()) {
		subobj = json_object_new_string(master_id.c_str());
		json_object_object_add(evobj, "masterId", subobj);
	}
	subobj = json_object_new_string(device_id.c_str());
	json_object_object_add(evobj, "rokidId", subobj);
	subobj = json_object_new_string(device_type_id.c_str());
	json_object_object_add(evobj, "rokidDtId", subobj);
	add_event_kvs(evobj, ev->kvs);
	json_object_array_add(data_array, evobj);
}

void JSONGenerator::end() {
	json_object_object_add(json_root_obj, "data", data_array);
	json_string = json_object_to_json_string_ext(json_root_obj, JSON_C_TO_STRING_PRETTY);
	json_object_put(json_root_obj);
	json_root_obj = nullptr;
	data_array = nullptr;
}

void JSONGenerator::gen_sign(const string& nonce, string& result) {
	string sign_src;
	sign_src = device_id;
	sign_src.append("apiVersion");
	sign_src.append(API_VERSION);
	sign_src.append("nonce");
	sign_src.append(nonce);
	sign_src.append(device_id);

	uint8_t md5_res[MD5_DIGEST_LENGTH];
	static char buf[64];
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, sign_src.c_str(), sign_src.length());
	MD5_Final(md5_res, &ctx);
	snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
			md5_res[0], md5_res[1], md5_res[2], md5_res[3],
			md5_res[4], md5_res[5], md5_res[6], md5_res[7],
			md5_res[8], md5_res[9], md5_res[10], md5_res[11],
			md5_res[12], md5_res[13], md5_res[14], md5_res[15]);
	result = buf;
}

void JSONGenerator::add_event_kvs(struct json_object* jobj, KeyValueArray& kvs) {
	size_t i;
	size_t size = kvs.size();
	struct json_object* sub;
	char buf[4];
	if (size > MAX_KVS)
		size = MAX_KVS;
	for (i = 0; i < size; ++i) {
		snprintf(buf, sizeof(buf), "k%d", (int32_t)i);
		sub = json_object_new_string(kvs[i].first.c_str());
		json_object_object_add(jobj, buf, sub);
		snprintf(buf, sizeof(buf), "v%d", (int32_t)i);
		sub = json_object_new_string(kvs[i].second.c_str());
		json_object_object_add(jobj, buf, sub);
	}
}

TraceUploader::TraceUploader(const string& dev_id, const string& dev_tp_id) {
	device_id = dev_id;
	device_type_id = dev_tp_id;
	Poco::Net::initializeSSL();
	context = new Context(Context::CLIENT_USE, "", "", "", Context::VERIFY_NONE);
	run_thread = thread([this]() { this->run(); });
}

TraceUploader::~TraceUploader() {
	ev_mutex.lock();
	working = 0;
	ev_cond.notify_one();
	ev_mutex.unlock();

	run_thread.join();
}

void TraceUploader::put(shared_ptr<TraceEvent>& ev) {
	lock_guard<mutex> locker(ev_mutex);
	trace_events.push_back(ev);
	ev_cond.notify_one();
}

void TraceUploader::run() {
	unique_lock<mutex> locker(ev_mutex);
	JSONGenerator json_gen(device_id, device_type_id);

	while (working) {
		if (trace_events.empty()) {
			ev_cond.wait(locker);
			continue;
		}

		json_gen.start();
		while (trace_events.size()) {
			json_gen.add_event(trace_events.front());
			trace_events.pop_front();
		}
		json_gen.end();

		locker.unlock();
		send_request(json_gen.json_str());
		locker.lock();
	}
}

#define RESP_BODY_BUF_LEN 1024
void TraceUploader::send_request(const string& body) {
	URI uri;

	KLOGD(TAG, "http body = %s", body.c_str());

	uri = SERVER_URI;
	HTTPSClientSession session(uri.getHost(), uri.getPort(), context);
	HTTPRequest req(HTTPRequest::HTTP_POST, uri.getPath(), HTTPMessage::HTTP_1_1);
	HTTPResponse resp;

	req.setContentLength(body.length());
	try {
		ostream& os = session.sendRequest(req);
		os.write(body.data(), body.length());
		istream& is = session.receiveResponse(resp);

		KLOGD(TAG, "http resp status = %d", resp.getStatus());
		static char resp_body[RESP_BODY_BUF_LEN];
		is.read(resp_body, RESP_BODY_BUF_LEN);
		KLOGD(TAG, "http resp body = %s", resp_body);
	} catch (Exception& e) {
		KLOGE(TAG, "send request exception %s", e.displayText().c_str());
		return;
	} catch (std::exception& e) {
		KLOGE(TAG, "send request exception %s", e.what());
		return;
	}
}

void TraceEvent::add_key_value(const string& key, const string& val) {
	kvs.emplace_back();
	kvs.back().first = key;
	kvs.back().second = val;
}

#endif // ROKID_UPLOAD_TRACE
