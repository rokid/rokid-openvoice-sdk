#include "voice_stream_reader.h"
#include "log.h"

using rokid::open::SpeechResponse;

namespace rokid {
namespace speech {

static const char* tag_ = "speech.VoiceStreamReader";

void VoiceStreamReader::prepare(SpeechResponseQueueType* queue) {
	responses_ = queue;
	pthread_mutex_init(&mutex_, NULL);
	pthread_cond_init(&cond_, NULL);
	pthread_cond_init(&work_complete_, NULL);
	work_ = true;
	thread_ = new thread([=] { run(); });
}

void VoiceStreamReader::close() {
	pthread_mutex_lock(&mutex_);
	work_ = false;
	pthread_cond_signal(&cond_);
	pthread_mutex_unlock(&mutex_);

	thread_->join();
	delete thread_;
	thread_ = NULL;

	pthread_mutex_destroy(&mutex_);
	pthread_cond_destroy(&cond_);
	pthread_cond_destroy(&work_complete_);
}

void VoiceStreamReader::set_stream(int32_t id, VoiceSpeechStreamSp stream) {
	pthread_mutex_lock(&mutex_);
	id_ = id;
	stream_ = stream;
	Log::d(tag_, "set stream, awake response thread");
	pthread_cond_signal(&cond_);
	pthread_mutex_unlock(&mutex_);
}

void VoiceStreamReader::wait_complete() {
	pthread_mutex_lock(&mutex_);
	if (stream_.get())
		pthread_cond_wait(&work_complete_, &mutex_);
	pthread_mutex_unlock(&mutex_);
}

void VoiceStreamReader::run() {
	SpeechResponse resp;
	shared_ptr<ResponseQueueItem> item;

	while (work_) {
		wait_stream_available();
		if (!work_) {
			Log::d(tag_, "work is false, will quit thread soon");
			work_complete();
			break;
		}

		Log::d(tag_, "before stream read");
		while (stream_->Read(&resp)) {
			item.reset(new ResponseQueueItem());
			item->type = 0;
			item->err = 0;
			item->asr = resp.asr();
			item->nlp = resp.nlp();
			item->action = resp.action();
			Log::d(tag_, "voice speech response id(%d), nlp(%s)", id_, item->nlp.c_str());
			responses_->add(id_, item);
		}
		Log::d(tag_, "voice speech response read end");

		work_complete();
	}
	Log::d(tag_, "quit voice stream reader thread");
}

void VoiceStreamReader::wait_stream_available() {
	pthread_mutex_lock(&mutex_);
	if (work_ && !stream_.get()) {
		Log::d(tag_, "response thread wait stream set");
		pthread_cond_wait(&cond_, &mutex_);
		Log::d(tag_, "response thread awake");
	}
	pthread_mutex_unlock(&mutex_);
}

void VoiceStreamReader::work_complete() {
	pthread_mutex_lock(&mutex_);
	stream_.reset();
	pthread_cond_signal(&work_complete_);
	pthread_mutex_unlock(&mutex_);
}

} // namespace speech
} // namespace rokid
