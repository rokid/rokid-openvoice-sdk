#include "speech.pb.h"
#include "speech_connection.h"
#include "core.h"
#include "log.h"

using std::string;
using std::shared_ptr;
using std::unique_ptr;
using std::thread;
using grpc::ClientContext;
using grpc::Status;
using rokid::open::SpeechHeader;
using rokid::open::TextSpeechRequest;
using rokid::open::VoiceSpeechRequest;
using rokid::open::SpeechResponse;

namespace rokid {
namespace speech {

static const char* tag_ = "speech.core";
static const char* default_voice_trigger = "若琪";

SpeechCore::SpeechCore() : context_(NULL), work_(false), next_id_(0) {
	pthread_mutex_init(&mutex_, NULL);
	pthread_cond_init(&cond_, NULL);
	config_ = new SpeechConfig();
}

SpeechCore::~SpeechCore() {
	pthread_mutex_destroy(&mutex_);
	pthread_cond_destroy(&cond_);
	release();
	if (config_)
		delete config_;
}

bool SpeechCore::prepare(const char* config_file) {
	create_threads();

	stub_ = SpeechConnection::connect(config_, "speech");
	if (stub_.get() == NULL) {
		Log::w(tag_, "speech connection failed");
		stop_threads();
		return false;
	}
	return true;
}

void SpeechCore::release() {
	if (work_) {
		text_requests_.clear();
		responses_.close();
		stop_threads();
		voice_stream_reader_.close();
		stub_.reset();
	}
}

int32_t SpeechCore::put_text(const char* text) {
	SpeechInfoSp req(new SpeechInfo());
	req->id = next_id();
	req->data.reset(new string(text));
	pthread_mutex_lock(&mutex_);
	text_requests_.push_back(req);
	Log::d(tag_, "put text %s, text req size is %d", text, text_requests_.size());
	pthread_cond_signal(&cond_);
	pthread_mutex_unlock(&mutex_);
	return req->id;
}

int32_t SpeechCore::start_voice() {
	int32_t id = next_id();
	pthread_mutex_lock(&mutex_);
	voice_requests_.start(id);
	Log::d(tag_, "start voice(%d), voice req size is %d", id, voice_requests_.size());
	pthread_cond_signal(&cond_);
	pthread_mutex_unlock(&mutex_);
	return id;
}

void SpeechCore::put_voice(int32_t id, const uint8_t* data, uint32_t length) {
	if (id <= 0 || data == NULL || length == 0)
		return;
	shared_ptr<string> v(new string((const char*)data, length));
	pthread_mutex_lock(&mutex_);
	if (voice_requests_.stream(id, v)) {
		Log::d(tag_, "put voice(%d), voice length %d", id, length);
		pthread_cond_signal(&cond_);
	} else
		Log::d(tag_, "put voice(%d) failed, this id not existed?", id);
	pthread_mutex_unlock(&mutex_);
}

void SpeechCore::end_voice(int32_t id) {
	pthread_mutex_lock(&mutex_);
	if (voice_requests_.end(id)) {
		Log::d(tag_, "end voice(%d)", id);
		pthread_cond_signal(&cond_);
	} else
		Log::d(tag_, "end voice(%d) failed", id);
	pthread_mutex_unlock(&mutex_);
}

void SpeechCore::cancel(int32_t id) {
	pthread_mutex_lock(&mutex_);
	if (voice_requests_.erase(id)) {
		Log::d(tag_, "cancel (%d)", id);
		pthread_cond_signal(&cond_);
	}
	pthread_mutex_unlock(&mutex_);
}

static int32_t item_type_to_poll_type(uint32_t type) {
	if (type >= 5)
		return -2;
	return (int32_t)type;
}

int32_t SpeechCore::poll(SpeechResult& res) {
	int32_t id;
	shared_ptr<ResponseQueueItem> item;
	if (!responses_.poll(id, item))
		return -1;
	res.id = id;
	res.err = item->err;
	res.asr = item->asr;
	res.nlp = item->nlp;
	res.action = item->action;
	Log::d(tag_, "poll: id(%d), type(%u), err(%u)\nasr %s\nnlp %s\naction %s",
			id, item->type, item->err,
			item->asr.c_str(), item->nlp.c_str(), item->action.c_str());
	return item_type_to_poll_type(item->type);
}

void SpeechCore::config(const char* key, const char* value) {
	if (config_)
		config_->set(key, value);
}

void SpeechCore::create_threads() {
	work_ = true;
	req_thread_ = new thread([=] { run_requests(); });
	voice_stream_reader_.prepare(&responses_);
}

void SpeechCore::stop_threads() {
	pthread_mutex_lock(&mutex_);
	work_ = false;
	pthread_cond_signal(&cond_);
	pthread_mutex_unlock(&mutex_);

	req_thread_->join();
	delete req_thread_;
	req_thread_ = NULL;
}

void SpeechCore::run_requests() {
	SpeechInfoSp req;
	VoiceSpeechInfo vreq;
	bool is_text = false;
	// 0: try poll text req, then try voice req
	// 1: only try poll voice req, until the voice req end
	uint32_t poll_flags = 0;
	VoiceSpeechStreamSp voice_stream;
	bool stream_broken = false;
	// debug
	uint32_t total_len = 0;

	while (true) {
		req.reset();

		// poll request
		pthread_mutex_lock(&mutex_);
		if (!work_) {
			pthread_mutex_unlock(&mutex_);
			break;
		}

		if (poll_flags == 0) {
			if (!text_requests_.empty()) {
				req = text_requests_.front();
				text_requests_.pop_front();
				is_text = true;
			}
		}
		if(req.get() == NULL && voice_requests_.available()) {
			VoiceSpeechInfo* vreq = new VoiceSpeechInfo();
			uint32_t err;
			vreq->type = voice_requests_.pop(vreq->id, vreq->data, err);
			assert(vreq->type >= 0);
			req.reset(vreq);
			if (vreq->type == 1)
				poll_flags = 1;
			else if (vreq->type >= 2)
				poll_flags = 0;
			is_text = false;
		}
		if (req.get() == NULL) {
			Log::d(tag_, "no request, block wait");
			pthread_cond_wait(&cond_, &mutex_);
			pthread_mutex_unlock(&mutex_);
			continue;
		}
		pthread_mutex_unlock(&mutex_);

		// do request
		if (is_text) {
			ClientContext ctx;
			TextSpeechRequest grpc_req;
			SpeechResponse grpc_resp;
			SpeechHeader* header = grpc_req.mutable_header();
			header->set_id(req->id);
			header->set_lang("zh");
			grpc_req.set_asr(*req->data);
			Log::d(tag_, "send text request %s", req->data->c_str());
			Status status = stub_->speecht(&ctx, grpc_req, &grpc_resp);
			shared_ptr<ResponseQueueItem> item(new ResponseQueueItem());
			if (status.ok()) {
				// success response
				item->type = 0;
				item->asr = grpc_resp.asr();
				item->nlp = grpc_resp.nlp();
				item->action = grpc_resp.action();
				Log::d(tag_, "text request ok. push response id %d, type %d",
						req->id, item->type);
				responses_.add(req->id, item);
			} else {
				Log::d(tag_, "text request failed: %d:%s",
						status.error_code(), status.error_message().c_str());
				// error response
				item->type = 4;
				// error code now always 1
				item->err = 1;
				Log::d(tag_, "push response id %d, type %d, err %d",
						req->id, item->type, item->err);
				responses_.add(req->id, item);
			}
		} else {
			VoiceSpeechInfo* vreq = (VoiceSpeechInfo*)req.get();
			VoiceSpeechRequest grpc_req;
			switch (vreq->type) {
				// voice data
				case 0:
					Log::d(tag_, "voice request send data");
					grpc_req.set_voice(*vreq->data);
					assert(voice_stream.get());
					total_len += vreq->data->length();
					Log::d(tag_, "stream ptr %p, write total length = %u", voice_stream.get(), total_len);
					if (!stream_broken && !voice_stream->Write(grpc_req)) {
						Log::d(tag_, "*b* write failed, stream broken");
						stream_broken = true;
					}
					Log::d(tag_, "*b* write finish");
					break;
				// voice start
				case 1: {
					Log::d(tag_, "voice request start, id %d", vreq->id);
					assert(context_ == NULL);
					context_ = new ClientContext();
					voice_stream = stub_->speechv(context_);
					stream_broken = false;
					total_len = 0;

					SpeechHeader* header = grpc_req.mutable_header();
					header->set_id(vreq->id);
					header->set_lang("zh");
					const char* codec = config_->get("codec", "pcm");
					header->set_codec(codec);
					const char* vt = config_->get("vt", default_voice_trigger);
					header->set_vt(vt);
					Log::d(tag_, "write header, id %d", vreq->id);
					if (!voice_stream->Write(grpc_req)) {
						Log::d(tag_, "*a* write failed, stream broken");
						stream_broken = true;
					}
					Log::d(tag_, "*a* write finish");

					shared_ptr<ResponseQueueItem> item(new ResponseQueueItem());
					item->type = 1;
					Log::d(tag_, "push response, type(%d)", item->type);
					responses_.add(vreq->id, item);

					voice_stream_reader_.set_stream(vreq->id, voice_stream);
					break;
				}
				// voice end
				case 2:
					assert(voice_stream.get());
				// voice cancel
				case 3: {
					Log::d(tag_, "voice request end or cancel(%d), id %d, stream ptr %p",
							vreq->type, vreq->id, voice_stream.get());
					Status status;
					if (voice_stream.get()) {
						voice_stream->WritesDone();
						Log::d(tag_, "wait voice stream Finish");
						status = voice_stream->Finish();
						Log::d(tag_, "voice stream Finish: %d, %s",
								status.error_code(), status.error_message().c_str());
						Log::d(tag_, "wait stream reader complete");
						voice_stream_reader_.wait_complete();
						Log::d(tag_, "stream reader complete");
						voice_stream.reset();
						delete context_;
						context_ = NULL;
					}

					shared_ptr<ResponseQueueItem> item(new ResponseQueueItem());
					if (status.ok()) {
						item->type = 2;
						item->err = 0;
					} else {
						item->type = 4;
						item->err = 1;
					}
					Log::d(tag_, "push response, type(%d), err(%d)", item->type, item->err);
					responses_.add(vreq->id, item);
					break;
				}
			}
		}
	}
}

Speech* new_speech() {
	return new SpeechCore();
}

void delete_speech(Speech* speech) {
	if (speech)
		delete speech;
}

} // namespace speech
} // namespace rokid
