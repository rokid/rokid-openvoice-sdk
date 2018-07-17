#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include "speech_stress_test.h"
#include "rlog.h"
#include "test_utils.h"

#define AUDIO_FILE "rkpcmrec"
#define PCM_FRAME_SIZE 320
// 10ms, 10000ns
#define PCM_FRAME_DURATION 10
#define TAG "SpeechStressTest"

using namespace rokid::speech;
using namespace std;
using namespace std::chrono;

void SpeechStressTest::test(const rokid::speech::PrepareOptions& opts, const char* pcm_file, uint32_t repeat) {
	uint32_t pcm_size;
	uint8_t* pcm_data = read_pcm_data(pcm_file, pcm_size);
	if (pcm_data == nullptr)
		return;
	shared_ptr<Speech> speech = Speech::new_instance();
	speech->prepare(opts);
	shared_ptr<SpeechOptions> sopts = SpeechOptions::new_instance();
	sopts->set_vad_mode(VadMode::CLOUD, 500);
	sopts->set_no_intermediate_asr(true);
	speech->config(sopts);

	SpeechStressTest testcase;
	testcase.run(speech, pcm_data, pcm_size, repeat);

	speech->release();
	release_pcm_data(pcm_data, pcm_size);
}

void SpeechStressTest::run(shared_ptr<Speech>& sp, uint8_t* pcm, uint32_t size, uint32_t repeat) {
	speech = sp;
	init();

	uint32_t pcm_offset;
	VoiceOptions vopt;
	vopt.trigger_confirm_by_cloud = 0;
	while (repeat) {
		--repeat;
		pcm_offset = 0;
		while (true) {
			if (cur_speech_id <= 0) {
				cur_speech_id = speech->start_voice(&vopt);
				if (cur_speech_id <= 0) {
					KLOGW(TAG, "speech start voice failed, return %d", cur_speech_id);
					goto exit;
				}
				add_speech_req(cur_speech_id);
				++speech_req_count;
			}

			if (size - pcm_offset < PCM_FRAME_SIZE)
				break;
			speech->put_voice(cur_speech_id, pcm + pcm_offset, PCM_FRAME_SIZE);
			pcm_offset += PCM_FRAME_SIZE;
			usleep(PCM_FRAME_DURATION * 1000);
		}
	}

	wait_pending_reqs();
	print_test_result();

exit:
	final();
}

void SpeechStressTest::init() {
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
	info.start_tp = SteadyClock::now();
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
			fs.duration = duration_cast<milliseconds>(SteadyClock::now() - reqinfo->start_tp);
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
	KLOGI(TAG, "==============speech stress test result===============");
	KLOGI(TAG, "speech request number %u, success %u", speech_req_count, speech_req_success);
	for (i = 0; i < failed_speech.size(); ++i) {
		KLOGI(TAG, "id: %lu  duration: %u  error: %d",
				failed_speech[i].id,
				failed_speech[i].duration.count(),
				failed_speech[i].error);
	}
}
