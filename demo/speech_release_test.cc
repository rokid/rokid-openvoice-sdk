#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <chrono>
#include "speech_release_test.h"
#include "test_utils.h"

using std::shared_ptr;
using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::seconds;
using std::thread;
using rokid::speech::SpeechResult;
using rokid::speech::SpeechResultType;
using rokid::speech::Codec;
using rokid::speech::VadMode;
using rokid::speech::SpeechOptions;

void SpeechReleaseTest::test(const PrepareOptions& opts, const DemoOptions& dopts) {
	uint32_t pcm_size = 0;
	uint8_t* pcm_data = nullptr;
	
	if (dopts.pcm) {
		pcm_data = read_pcm_data(dopts.pcm, pcm_size);
		if (pcm_data == nullptr)
			return;
	}
	SpeechReleaseTest test(opts, pcm_data, pcm_size);
	test.run(dopts);
	release_pcm_data(pcm_data, pcm_size);
}

SpeechReleaseTest::SpeechReleaseTest(const PrepareOptions& opts, const uint8_t* p, uint32_t size)
	: prepare_options(&opts), pcm(p), pcm_size(size), flags(0), speech_id(-1) {
	speech = Speech::new_instance();
}

void SpeechReleaseTest::run(const DemoOptions& dopts) {
	uint32_t min_run_dur = dopts.min_release_interval;
	uint32_t max_run_dur = dopts.max_release_interval;
	steady_clock::time_point begin = steady_clock::now();
	steady_clock::time_point cur;
	uint32_t mod = max_run_dur - min_run_dur;
	shared_ptr<SpeechOptions> sopts = SpeechOptions::new_instance();
	sopts->set_codec(Codec::PCM);
	sopts->set_vad_mode(VadMode::CLOUD, 500);

	srand(duration_cast<microseconds>(begin.time_since_epoch()).count());
	create_work_threads();
	while (true) {
		speech->prepare(*prepare_options);
		speech->config(sopts);
		flags = 1;
		usleep((min_run_dur + rand() % mod) * 1000);
		flags = 0;
		speech->release();

		cur = steady_clock::now();
		if (duration_cast<seconds>(cur - begin).count() >= dopts.test_duration) {
			flags = 2;
			break;
		}
	}
	destroy_work_threads();
}

void SpeechReleaseTest::create_work_threads() {
	if (pcm_size > 0) {
		put_thread = new thread([this] () {
				this->do_speech_putvoice();
			});
		poll_thread = new thread([this] () {
				this->do_speech_poll();
			});
	}
}

void SpeechReleaseTest::destroy_work_threads() {
	if (pcm_size > 0) {
		put_thread->join();
		poll_thread->join();
		delete put_thread;
		delete poll_thread;
	}
}

void SpeechReleaseTest::do_speech_putvoice() {
	uint32_t sample_rate = 16000;
	uint32_t bytes_per_sample = 2;
	uint32_t frame_duration = 10;
	uint32_t frame_size = sample_rate * bytes_per_sample * frame_duration / 1000;
	uint32_t pcm_offset = 0;

	while (true) {
		switch (flags) {
			case 0:
				sleep(1);
				break;
			case 1:
				if (speech_id < 0) {
					speech_id = speech->start_voice();
					printf("new speech id %d\n", speech_id);
				}
				if (speech_id > 0) {
					if (pcm_size - pcm_offset >= frame_size) {
						speech->put_voice(speech_id, pcm + pcm_offset, frame_size);
						pcm_offset += frame_size;
						usleep(frame_duration * 1000);
					} else {
						speech->end_voice(speech_id);
						pcm_offset = 0;
						printf("speech end voice, wait speech id clear\n");
						while (speech_id > 0)
							sleep(1);
						printf("speech end voice, speech id was clear\n");
					}
				}
				break;
			case 2:
				printf("thread 'do_speech_putvoice' quit\n");
				return;
		}
	}
}

void SpeechReleaseTest::do_speech_poll() {
	SpeechResult result;

	while (true) {
		if (flags == 0) {
			sleep(1);
		} else if (flags == 1) {
			if (!speech->poll(result)) {
				speech_id = -1;
				continue;
			}
			if (result.type == SpeechResultType::SPEECH_RES_END
					|| result.type == SpeechResultType::SPEECH_RES_CANCELLED
					|| result.type == SpeechResultType::SPEECH_RES_ERROR)
				speech_id = -1;
		} else if (flags == 2) {
			printf("thread 'do_speech_poll' quit\n");
			break;
		}
	}
}
