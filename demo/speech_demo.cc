#include "speech.h"
#include "common.h"

using namespace rokid::speech;

static int32_t last_id_ = 0;
static bool quit_ = false;

static void speech_req(Speech* speech) {
	speech->put_text("你好");
	speech->put_text("你好");
	speech->put_text("你好");
	int32_t id = speech->put_text("你好");
	speech->put_text("你好");
	speech->put_text("你好");
	last_id_ = speech->put_text("你好");
	speech->cancel(id);
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
		switch (res.type) {
			case 0:
				printf("%u: recv speech asr %s\nnlp %s\naction %s\n",
						res.id, res.asr.c_str(), res.nlp.c_str(),
						res.action.c_str());
				break;
			case 1:
				printf("%u: speech onStart\n", res.id);
				break;
			case 2:
				printf("%u: speech onComplete\n", res.id);
				speech_req_done(res.id);
				break;
			case 3:
				printf("%u: speech onStop\n", res.id);
				speech_req_done(res.id);
				break;
			case 4:
				printf("%u: speech onError %u\n", res.id, res.err);
				speech_req_done(res.id);
				break;
		}
	}
	speech->release();
}

void speech_demo() {
	Speech* speech = new_speech();
	prepare(speech);
	speech->config("codec", "opu2");
	run(speech, speech_req, speech_poll);
	delete_speech(speech);
}
