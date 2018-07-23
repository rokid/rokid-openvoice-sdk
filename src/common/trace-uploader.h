#pragma once

#ifdef ROKID_UPLOAD_TRACE

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Poco/Net/Context.h"

#define TRACE_EVENT_TYPE_BASE 0
#define TRACE_EVENT_TYPE_SYS 1
#define TRACE_EVENT_TYPE_APP 2
#define TRACE_EVENT_TYPE_LOG 3

typedef std::vector<std::pair<std::string, std::string> > KeyValueArray;

class TraceEvent {
public:
	void add_key_value(const std::string& key, const std::string& val);

public:
	int32_t type = TRACE_EVENT_TYPE_LOG;
	std::string id;
	std::string name;
	KeyValueArray kvs;
};

typedef std::list<std::shared_ptr<TraceEvent> > TraceEventQueue;

class TraceUploader {
public:
	TraceUploader(const std::string& dev_id, const std::string& dev_tp_id);

	~TraceUploader();

	void put(std::shared_ptr<TraceEvent>& ev);

private:
	void run();

	void send_request(const std::string& body);

private:
	std::mutex ev_mutex;
	std::condition_variable ev_cond;
	TraceEventQueue trace_events;
	std::thread run_thread;
	std::string device_id;
	std::string device_type_id;
	Poco::Net::Context::Ptr context;
	int32_t working = 1;
};

#endif // ROKID_UPLOAD_TRACE
