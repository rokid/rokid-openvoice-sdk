#include "asr.h"
#include "common.h"

using namespace rokid::speech;

static int32_t last_id_ = 0;
static bool quit_ = false;
static const char* OPU_FILE = "demo/hello.opu";

static void asr_req(Asr* asr) {
	FILE* fp;
	fp = fopen(OPU_FILE, "r");
	if (fp == NULL) {
		printf("open %s failed\n", OPU_FILE);
		return;
	}
	int32_t id = asr->start();
	uint8_t buf[256];
	while (true) {
		size_t r = fread(buf, 1, sizeof(buf), fp);
		if (r > 0)
			asr->voice(id, buf, r);
		if (r < sizeof(buf))
			break;
	}
	fclose(fp);
	asr->end(id);
	last_id_ = id;
}

static void asr_req_done(int32_t id) {
	if (id == last_id_)
		quit_ = true;
}

static void asr_poll(Asr* asr) {
	AsrResult res;
	while (!quit_) {
		if (!asr->poll(res))
			break;
		switch (res.type) {
			case 0:
				printf("%u: recv asr %s\n", res.id, res.asr.c_str());
				break;
			case 1:
				printf("%u: asr onStart\n", res.id);
				break;
			case 2:
				printf("%u: asr onComplete\n", res.id);
				asr_req_done(res.id);
				break;
			case 3:
				printf("%u: asr onStop\n", res.id);
				asr_req_done(res.id);
				break;
			case 4:
				printf("%u: asr onError %u\n", res.id, res.err);
				asr_req_done(res.id);
				break;
		}
	}
	asr->release();
}

void asr_demo() {
	Asr* asr = new_asr();
	prepare(asr);
	asr->config("codec", "opu");
	run(asr, asr_req, asr_poll);
	delete_asr(asr);
}
