#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <chrono>
#include "speech_stress_test.h"
#include "log.h"

#define AUDIO_FILE "rkpcmrec"
#define PCM_FRAME_SIZE 320
// 10ms, 10000ns
#define PCM_FRAME_DURATION 10000
#define TAG "SpeechStressTest"

using namespace rokid::speech;
using namespace std;
using namespace std::chrono;

void SpeechStressTest::run(const PrepareOptions& opts, uint32_t repeat) {
	init(opts);

	FILE* fp = fopen(AUDIO_FILE, "r");
	if (fp == NULL) {
		Log::w(TAG, "cannot open audio file %s", AUDIO_FILE);
		return;
	}
	uint32_t pcm_file_size;
	uint32_t pcm_offset;
	fseek(fp, 0, SEEK_END);
	pcm_file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	uint8_t pcmbuf[PCM_FRAME_SIZE];
	size_t c;
	steady_clock::time_point prev_tp = steady_clock::now();
	steady_clock::time_point tmp;
	nanoseconds dur;
	while (repeat) {
		--repeat;
		pcm_offset = 0;
		while (true) {
			if (cur_speech_id <= 0) {
				cur_speech_id = speech->start_voice();
				if (cur_speech_id <= 0) {
					Log::w(TAG, "speech start voice failed, return %d", cur_speech_id);
					goto exit;
				}
				add_speech_req(cur_speech_id);
				++speech_req_count;
			}
			c = fread(pcmbuf, 1, PCM_FRAME_SIZE, fp);
			if (c == 0) {
				fseek(fp, 0, SEEK_SET);
				break;
			}
			pcm_offset += c;
//			tmp = steady_clock::now();
//			dur = duration_cast<nanoseconds>(tmp - prev_tp);
//			prev_tp = tmp;
#ifdef SPEECH_SDK_DETAIL_TRACE
			Log::d(TAG, "send pcm offset %u/%u", pcm_offset, pcm_file_size);
#endif
			speech->put_voice(cur_speech_id, pcmbuf, c);
//			if (dur.count() < PCM_FRAME_DURATION) {
//				usleep(PCM_FRAME_DURATION - dur.count());
//			}
			usleep(PCM_FRAME_DURATION);
		}
	}

	wait_pending_reqs();
	print_test_result();

exit:
	final();
}

void SpeechStressTest::init(const PrepareOptions& opts) {
	speech = Speech::new_instance();
	speech->prepare(opts);
	shared_ptr<SpeechOptions> sopts = SpeechOptions::new_instance();
	sopts->set_vad_mode(VadMode::CLOUD, 500);
	sopts->set_no_intermediate_asr(true);
	speech->config(sopts);
	poll_thread = new thread([=] { run_speech_poll(); });
	cur_speech_id = 0;
	speech_req_count = 0;
	speech_req_success = 0;
}

void SpeechStressTest::final() {
	speech->release();
	poll_thread->join();
	delete poll_thread;
	poll_thread = NULL;
	failed_speech.clear();
	pending_speech_reqs.clear();
}

void SpeechStressTest::add_speech_req(int32_t id) {
	SpeechReqInfo info;
	info.id = id;
	info.start_tp = steady_clock::now();
	lock_guard<mutex> locker(list_mutex);
	pending_speech_reqs.push_back(info);
}

void SpeechStressTest::run_speech_poll() {
	SpeechResult result;
	SpeechReqInfo* reqinfo;

	while (true) {
		if (!speech->poll(result))
			break;
		reqinfo = get_top_speech_req();
		assert(reqinfo);
		handle_speech_result(reqinfo, result);
	}
}

SpeechReqInfo* SpeechStressTest::get_top_speech_req() {
	lock_guard<mutex> locker(list_mutex);
	return &pending_speech_reqs.front();
}

void SpeechStressTest::pop_speech_req() {
	lock_guard<mutex> locker(list_mutex);
	pending_speech_reqs.pop_front();
}

void SpeechStressTest::handle_speech_result(SpeechReqInfo* reqinfo,
		const SpeechResult& result) {
	FailedSpeech fs;
	switch (result.type) {
		case SPEECH_RES_END:
			assert(reqinfo->id == result.id);
			pop_speech_req();
			cur_speech_id = 0;
			++speech_req_success;
			break;
		case SPEECH_RES_ERROR:
			assert(reqinfo->id == result.id);
			fs.id = result.id;
			fs.duration = duration_cast<milliseconds>(steady_clock::now() - reqinfo->start_tp);
			fs.error = result.err;
			failed_speech.push_back(fs);
			pop_speech_req();
			cur_speech_id = 0;
			break;
	}
}

void SpeechStressTest::wait_pending_reqs() {
	while (pending_speech_reqs.size())
		sleep(1);
}

void SpeechStressTest::print_test_result() {
	size_t i;
	Log::i(TAG, "==============speech stress test result===============");
	Log::i(TAG, "speech request number %u, success %u", speech_req_count, speech_req_success);
	for (i = 0; i < failed_speech.size(); ++i) {
		Log::i(TAG, "id: %lu  duration: %u  error: %d",
				failed_speech[i].id,
				failed_speech[i].duration.count(),
				failed_speech[i].error);
	}
}
