#include <time.h>
#include <stdlib.h>
#include "speech.h"
#include "common.h"
#include "log.h"

using namespace rokid::speech;

#define OPU_FILE_COUNT 12
#ifdef ANDROID
#define OPU_RES_DIR "/data/speech_demo"
#else
#define OPU_RES_DIR "./demo/res"
#endif

static int32_t last_id_ = 0xffffffff;
static bool quit_ = false;

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
	return id;
}

static void speech_req(Speech* speech) {
	uint32_t i;
	int32_t id;
	for (i = 0; i < 100; ++i) {
		one_speech(speech, 1);
		one_speech(speech, 3);
		one_speech(speech, 1);
		one_speech(speech, 4);
		one_speech(speech, 2);
		one_speech(speech, 1);
		one_speech(speech, 2);
		one_speech(speech, 4);
		id = one_speech(speech, 3);
		Log::i("demo.speech.info", "9 speech req loop finish, idx %d", i);
		if (rand() & 1)
			speech->cancel(id);
	}
	last_id_ = one_speech(speech, 1);

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
				Log::i("demo.speech.info", "%u: speech onStart", res.id);
				break;
			case 2:
				Log::i("demo.speech.info", "%u: speech onComplete", res.id);
				speech_req_done(res.id);
				break;
			case 3:
				Log::i("demo.speech.info", "%u: speech onCancel", res.id);
				speech_req_done(res.id);
				break;
			case 4:
				Log::i("demo.speech.info", "%u: speech onError %u", res.id, res.err);
				speech_req_done(res.id);
				break;
		}
	}
	Log::d("demo.speech", "speech demo quit %d, release speech sdk", quit_);
	speech->release();
}

void speech_demo() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	srand(ts.tv_nsec);

	Speech* speech = new_speech();
	prepare(speech);
	speech->config("codec", "opu");
	run(speech, speech_req, speech_poll);

	last_id_ = 0xffffffff;
	quit_ = false;
	prepare(speech);
	run(speech, speech_req, speech_poll);

	delete_speech(speech);
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
