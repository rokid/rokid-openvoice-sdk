#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include "speech.h"
#include "common.h"
#include "log.h"

#ifdef __MACH__
#include <sys/time.h>
#else
#include <time.h>
#endif

using namespace rokid::speech;
using std::vector;
using std::shared_ptr;

#define OPU_FILE_COUNT 12
#ifdef ANDROID
#define OPU_RES_DIR "/data/speech_demo"
#else
#define OPU_RES_DIR "./demo/res"
#endif
#define TOTAL_REQ_NUM 100

static int32_t last_id_ = TOTAL_REQ_NUM;
static bool quit_ = false;

class SpeechRespStatus {
public:
	uint8_t start_count;
	uint8_t complete_count;
	uint8_t cancel_count;
	uint8_t error_count;

	SpeechRespStatus() : start_count(0), complete_count(0)
						, cancel_count(0), error_count(0) {
	}
};
static vector<SpeechRespStatus> all_resp_status_;

static void send_opu_file(Speech* speech, int32_t id, uint32_t idx) {
	char fname[64];
	char *buf;
	snprintf(fname, sizeof(fname), "%s/voice1_%u.opu", OPU_RES_DIR, idx);
	FILE* f = fopen(fname, "r");
	size_t s;

	if (f == NULL) {
		Log::d("demo.speech", "open opu file %s failed", fname);
		return;
	}
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
	buf = new char[size];
	s = fread(buf, size, 1, f);
	Log::d("demo.speech", "read voice file %s, %d[%d] bytes", fname, size, s);
	if (s > 0)
		speech->put_voice(id, (uint8_t*)buf, size);
	fclose(f);
}

static int32_t one_speech(Speech* speech, uint32_t repeat) {
	uint32_t i;
	int32_t id;
	id = speech->start_voice();
	while (repeat > 0) {
		for (i = 0; i < OPU_FILE_COUNT; ++i)
			send_opu_file(speech, id, i);
		--repeat;
	}
	speech->end_voice(id);
	Log::i("demo.speech.info", "one speech req finish, id %d", id);
	sleep(1);
	return id;
}

static void speech_req(Speech* speech) {
	uint32_t i;
	int32_t id;
	for (i = 0; i < TOTAL_REQ_NUM; ++i) {
		id = one_speech(speech, rand() % 4 + 1);
		if (rand() & 1)
			speech->cancel(id);
	}

	/**
	speech->put_text("你好");
	speech->put_text("你好");
	speech->put_text("你好");
	int32_t id = speech->put_text("你好");
	speech->put_text("你好");
	speech->put_text("你好");
	last_id_ = speech->put_text("你好");
	speech->cancel(id);
	*/
}

static void speech_req_done(int32_t id) {
	if (id == last_id_)
		quit_ = true;
}

static void speech_poll(Speech* speech) {
	SpeechResult res;
	while (!quit_) {
		if (!speech->poll(res))
			break;
		if (rand() & 1)
			speech->cancel(res.id);
		switch (res.type) {
			case 0:
				Log::i("demo.speech.info", "%u: recv speech asr %s\nnlp %s\naction %s\n",
						res.id, res.asr.c_str(), res.nlp.c_str(),
						res.action.c_str());
				break;
			case 1:
				if (res.id > 0 && res.id < all_resp_status_.size())
					++all_resp_status_[res.id].start_count;
				Log::i("demo.speech.info", "%u: speech onStart", res.id);
				break;
			case 2:
				if (res.id > 0 && res.id < all_resp_status_.size())
					++all_resp_status_[res.id].complete_count;
				Log::i("demo.speech.info", "%u: speech onComplete", res.id);
				speech_req_done(res.id);
				break;
			case 3:
				if (res.id > 0 && res.id < all_resp_status_.size())
					++all_resp_status_[res.id].cancel_count;
				Log::i("demo.speech.info", "%u: speech onCancel", res.id);
				speech_req_done(res.id);
				break;
			case 4:
				if (res.id > 0 && res.id < all_resp_status_.size())
					++all_resp_status_[res.id].error_count;
				Log::i("demo.speech.info", "%u: speech onError %u", res.id, res.err);
				speech_req_done(res.id);
				break;
		}
	}
	Log::d("demo.speech", "speech demo quit %d, release speech sdk", quit_);
	speech->release();
}

static void trace_resp_status() {
	uint32_t i;
	for (i = 0; i <= TOTAL_REQ_NUM; ++i) {
		if (all_resp_status_[i].start_count > 1) {
			Log::i("demo.speech.respstatus", "[%d]: start count %u",
					i, all_resp_status_[i].start_count);
			continue;
		}
		if (all_resp_status_[i].complete_count
				+ all_resp_status_[i].cancel_count
				+ all_resp_status_[i].error_count > 1) {
			Log::i("demo.speech.respstatus", "[%d]: complete count %u, "
					"cancel count %u, error count %u", i,
					all_resp_status_[i].complete_count,
					all_resp_status_[i].cancel_count,
					all_resp_status_[i].error_count);
		}
	}
}

void speech_demo() {
#ifdef __MACH__
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	srand(ts.tv_nsec);
#endif

	all_resp_status_.resize(TOTAL_REQ_NUM + 1);
	shared_ptr<Speech> speech = new_speech();
	prepare(speech.get());
	speech->config("codec", "opu");
	run(speech.get(), speech_req, speech_poll);

	trace_resp_status();
	/**
	Speech* speech = new_speech();
	prepare(speech);
	speech->release();
	int i;
	for (i=0; i<100; ++i) {
		printf("loop %d, prepare speech", i);
		speech->prepare();
		speech->release();
		printf("loop %d, release speech", i);
	}
	*/
}
