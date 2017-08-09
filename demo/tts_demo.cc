#include <unistd.h>
#include "tts.h"
#include "common.h"

using namespace rokid::speech;
using std::shared_ptr;

static int32_t last_id_ = 0xffffffff;
static bool quit_ = false;

static void tts_req(Tts* tts) {
	int32_t id;
	id = tts->speak("你好");
	sleep(1);
	id = tts->speak("你好");
	usleep(10000);
	tts->cancel(id);
	tts->speak("你好");
	id = tts->speak("你好");
	tts->speak("你好");
	tts->speak("你好");
	last_id_ = tts->speak("你好");
	tts->cancel(id);
}

static void tts_req_done(int32_t id) {
	if (id == last_id_)
		quit_ = true;
}

static void tts_poll(Tts* tts) {
	TtsResult res;
	while (!quit_) {
		if (!tts->poll(res))
			break;
		switch (res.type) {
			case 0:
				printf("%u: recv tts data %zd bytes\n", res.id, res.voice->length());
				break;
			case 1:
				printf("%u: tts onStart\n", res.id);
				break;
			case 2:
				printf("%u: tts onComplete\n", res.id);
				tts_req_done(res.id);
				break;
			case 3:
				printf("%u: tts onStop\n", res.id);
				tts_req_done(res.id);
				break;
			case 4:
				printf("%u: tts onError %u\n", res.id, res.err);
				tts_req_done(res.id);
				break;
		}
	}
	tts->release();
}

void tts_demo() {
	shared_ptr<Tts> tts = Tts::new_instance();
	prepare(tts.get());
	shared_ptr<TtsOptions> opt = TtsOptions::new_instance();
	opt->set_codec(Codec::OPU2);
	tts->config(opt);
	run(tts.get(), tts_req, tts_poll);
}
