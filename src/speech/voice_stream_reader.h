#pragma once

#include <thread>
#include <memory>
#include <grpc++/grpc++.h>
#include "speech.pb.h"
#include "speech.grpc.pb.h"
#include "pending_queue.h"

typedef std::shared_ptr<grpc::ClientReaderWriter<rokid::open::VoiceSpeechRequest,
		rokid::open::SpeechResponse> > VoiceSpeechStreamSp;

namespace rokid {
namespace speech {

class ResponseQueueItem {
public:
	// 0: data
	// 1: start
	// 2: end
	// 3: delete
	// 4: error
	uint32_t type;
	uint32_t err;
	string asr;
	string nlp;
	string action;
};

typedef PendingQueue<ResponseQueueItem> SpeechResponseQueueType;

class VoiceStreamReader {
public:
	void prepare(SpeechResponseQueueType* queue);

	void close();

	void set_stream(int32_t id, VoiceSpeechStreamSp stream);

	void wait_complete();

private:
	void run();

	void wait_stream_available();

	void work_complete();

private:
	std::thread* thread_;
	SpeechResponseQueueType* responses_;
	pthread_mutex_t mutex_;
	pthread_cond_t cond_;
	pthread_cond_t work_complete_;
	int32_t id_;
	VoiceSpeechStreamSp stream_;
	bool work_;
};

} // namespace speech
} // namespace rokid
