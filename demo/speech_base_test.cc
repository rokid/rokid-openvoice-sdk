#include <unistd.h>
#include <thread>
#include "speech_base_test.h"
#include "simple_wave.h"
#include "test_utils.h"
#include "rlog.h"

using namespace rokid::speech;
using namespace std;

#define TAG "SpeechBaseTest"

void SpeechBaseTest::run(const PrepareOptions& opt, const uint8_t* data, uint32_t size) {
	shared_ptr<Speech> speech = Speech::new_instance();
	shared_ptr<SpeechOptions> sopts = SpeechOptions::new_instance();
	speech->prepare(opt);
	sopts->set_vad_mode(VadMode::CLOUD, 500);
	speech->config(sopts);

	thread poll_thread(
			[this, &speech]() {
				this->speech_poll_routine(speech);
			});

  do_request(0, data, size, speech);

	speech->release();
	poll_thread.join();
}

void SpeechBaseTest::do_request(uint32_t off, const uint8_t* data,
                                uint32_t size, shared_ptr<Speech> &speech) {
	requiring = true;
	uint32_t frame_size = 16000 * 2 * 80 / 1000;
	int32_t id = speech->start_voice();
	while (off < size) {
		if (size - off < frame_size) {
			speech->end_voice(id);
			break;
		}
		speech->put_voice(id, data + off, frame_size);
		off += frame_size;
		usleep(80 * 1000);
		if (!requiring)
			break;
	}
}

void SpeechBaseTest::speech_poll_routine(shared_ptr<Speech>& speech) {
	SpeechResult result;

	while (true) {
		if (!speech->poll(result))
			break;
		switch (result.type) {
			case SPEECH_RES_ASR_FINISH:
				KLOGI(TAG, "asr is %s", result.asr.c_str());
        requiring = false;
        break;
			case SPEECH_RES_ERROR:
				KLOGI(TAG, "speech error %d", result.err);
        requiring = false;
        break;
		}
	}
}

void SpeechBaseTest::test(const PrepareOptions& opts, const DemoOptions& dopts) {
	SimpleWaveReader wav_reader;
	const uint8_t* data;
	uint32_t size;

	if (dopts.wav) {
		if (!wav_reader.open(dopts.wav)) {
			KLOGE(TAG, "open wav file %s failed", dopts.wav);
			return;
		}
		if (wav_reader.bits_per_sample() != 16) {
			KLOGE(TAG, "wav file %s format incorrect, bits is %d",
					dopts.wav, wav_reader.bits_per_sample());
			return;
		}
		if (wav_reader.sample_rate() != 16000) {
			KLOGE(TAG, "wav file %s format incorrect, sample rate is %d",
					dopts.wav, wav_reader.sample_rate());
			return;
		}
		data = reinterpret_cast<const uint8_t*>(wav_reader.data());
		size = wav_reader.size();
	} else if (dopts.pcm) {
		data = read_pcm_data(dopts.pcm, size);
	}

	SpeechBaseTest test;
	test.run(opts, data, size);
	if (dopts.wav)
		wav_reader.close();
	else
		release_pcm_data(const_cast<uint8_t*>(data), size);
}
