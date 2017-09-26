#include <thread>
#include <unistd.h>
#include "tts_mem_test.h"

static const char* tts_texts_[] = {
	"动态查看一个进程的内存使用",
	"对设备喊一下若琪，现在没有唤醒声音了吗？只是灯亮的话，感觉还得看着灯，好不方便。",
	"算法还存在问题",
	"参见体验问题邮件",
	"编译通过，运行还有问题。本周要解决运行问题",
};

void TtsMemTest::run() {
	shared_ptr<Tts> tts = Tts::new_instance();
	PrepareOptions opts;
	opts.host = "apigwws-daily.open.rokid.com";
	opts.port = 443;
	opts.branch = "/api";
	opts.key = "rokid_test_key";
	opts.device_type_id = "rokid_test_device_type_id";
	opts.secret = "rokid_test_secret";
	opts.device_id = "ming_pc";
	tts->prepare(opts);
	_tts = tts;

	thread poll_thread([=] { tts_poll(); });

	uint32_t idx = 0;
	uint32_t text_num = sizeof(tts_texts_) / sizeof(void*);
	while (true) {
		// tts->speak(tts_texts_[idx % text_num]);
		usleep(500000);
	}
}

void TtsMemTest::tts_poll() {
	TtsResult result;
	while (true) {
		if (!_tts->poll(result))
			break;
	}
}
