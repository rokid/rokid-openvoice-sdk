#pragma once

#include <thread>
#include <memory>
#include <list>
#include <pthread.h>
#include <grpc++/client_context.h>
#include "speech.grpc.pb.h"
#include "speech_config.h"
#include "pending_queue.h"
#include "speech.h"
#include "voice_stream_reader.h"

namespace rokid {
namespace speech {

struct SpeechInfo {
	int32_t id;
	std::shared_ptr<std::string> data;
};

struct VoiceSpeechInfo : public SpeechInfo {
	// 0: voice
	// 1: start
	// 2: end
	// 3: delete
	int32_t type;
};

class SpeechCore : public rokid::speech::Speech {
public:
	typedef std::shared_ptr<SpeechInfo> SpeechInfoSp;

	SpeechCore();

	~SpeechCore();

	bool prepare(const char* config_file);

	void release();

	int32_t put_text(const char* text);

	int32_t start_voice();

	void put_voice(int32_t id, const uint8_t* data, uint32_t length);

	void end_voice(int32_t id);

	void cancel(int32_t id);

	int32_t poll(SpeechResult& res);

	void config(const char* key, const char* value);

private:
	void create_threads();

	void stop_threads();

	void run_requests();

	inline int32_t next_id() { return ++next_id_; }

private:
	grpc::ClientContext* context_;
	std::unique_ptr<rokid::open::Speech::Stub> stub_;
	std::thread* req_thread_;

	std::list<SpeechInfoSp> text_requests_;
	StreamQueue<std::string> voice_requests_;
	pthread_mutex_t mutex_;
	pthread_cond_t cond_;
	PendingQueue<ResponseQueueItem> responses_;
	VoiceStreamReader voice_stream_reader_;
	SpeechConfig* config_;

	bool work_;
	int32_t next_id_;
};

} // namespace speech
} // namespace rokid
