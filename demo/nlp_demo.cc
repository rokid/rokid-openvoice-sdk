#include "nlp.h"
#include "common.h"

using namespace rokid::speech;

static int32_t last_id_ = 0;
static bool quit_ = false;

static void nlp_req(Nlp* nlp) {
	nlp->request("你好");
	nlp->request("你好");
	nlp->request("你好");
	int32_t id = nlp->request("你好");
	nlp->request("你好");
	nlp->request("你好");
	last_id_ = nlp->request("你好");
	nlp->cancel(id);
}

static void nlp_req_done(int32_t id) {
	if (id == last_id_)
		quit_ = true;
}

static void nlp_poll(Nlp* nlp) {
	NlpResult res;
	while (!quit_) {
		if (!nlp->poll(res))
			break;
		switch (res.type) {
			case 0:
				printf("%u: recv nlp data %s\n", res.id, res.nlp.c_str());
				nlp_req_done(res.id);
				break;
			case 1:
				printf("%u: nlp onStop\n", res.id);
				nlp_req_done(res.id);
				break;
			case 2:
				printf("%u: nlp onError %u\n", res.id, res.err);
				nlp_req_done(res.id);
				break;
		}
	}
	nlp->release();
}

void nlp_demo() {
	Nlp* nlp = new_nlp();
	prepare(nlp);
	nlp->config("codec", "opu2");
	run(nlp, nlp_req, nlp_poll);
	delete_nlp(nlp);
}
