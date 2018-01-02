#include <stdio.h>
#include <unistd.h>
#include <thread>
#include <string.h>
#include "tts.h"
#include "speech.h"
#include "simple_wave.h"
#ifdef HAS_OPUS_CODEC
#include <sys/mman.h>
#include "rkcodec.h"
#endif

using namespace rokid::speech;
using std::shared_ptr;
using std::string;

#define AUDIO_FILE_NAME_FORMAT "data%d.wav"

static const char* test_text[] = {
	"你好若琪，我是你爹",
	"只要998，若琪抱回家。998你买不了吃亏，998你买不了上当",
	"存在一个公式，形式系统既不能推出它，也不能推出它的否定，即形式系统无法判定它"
};

#if defined(TEST_MP3)
static FILE* mp3_fp = NULL;
void create_mp3_file(int32_t id) {
	char fname[32];
	snprintf(fname, sizeof(fname), "data%d.mp3", id);
	mp3_fp = fopen(fname, "w+");
}

void mp3_write(const string& data) {
	if (mp3_fp == NULL)
		return;
	fwrite(data.data(), data.length(), 1, mp3_fp);
}

void close_mp3_file() {
	if (mp3_fp) {
		fclose(mp3_fp);
		mp3_fp = NULL;
	}
}
#elif defined(HAS_OPUS_CODEC)
static RKOpusDecoder rkdecoder;
static void decode_write(const string& data, SimpleWaveWriter& writer) {
	const char* opu = data.data();
	uint32_t off = 0;
	const uint16_t* pcm_buf;
	while (off < data.length()) {
		pcm_buf = rkdecoder.decode_frame(opu + off);
		if (pcm_buf == NULL)
			break;
		writer.write(pcm_buf, rkdecoder.pcm_frame_size() * sizeof(uint16_t));
		off += rkdecoder.opu_frame_size();
	}
}
#endif

#ifdef HAS_OPUS_CODEC
static RKOpusEncoder rkencoder;
#define ENCODE_FRAMES 50
#endif

static void test_tts(PrepareOptions& opts) {
	shared_ptr<Tts> tts = Tts::new_instance();
	int speakCount = sizeof(test_text) / sizeof(char*);
	bool thflag = false;

	tts->prepare(opts);
	shared_ptr<TtsOptions> tts_opts = TtsOptions::new_instance();
#if defined(TEST_MP3)
	tts_opts->set_codec(Codec::MP3);
#elif defined(HAS_OPUS_CODEC)
	tts_opts->set_codec(Codec::OPU2);
#endif
	tts->config(tts_opts);
	std::thread poll_thread([&tts, &thflag, speakCount]() {
			int count = 0;
			TtsResult result;
			char fname[32];
			SimpleWaveWriter writer;

			thflag = true;
#if !defined(TEST_MP3) && defined(HAS_OPUS_CODEC)
			rkdecoder.init(24000, 16000, 20);
#endif
			while (count < speakCount) {
				if (!tts->poll(result))
					break;
				switch (result.type) {
				case TTS_RES_START:
					printf("speak %d start\n", result.id);
#ifdef TEST_MP3
					create_mp3_file(result.id);
#else
					snprintf(fname, sizeof(fname), AUDIO_FILE_NAME_FORMAT, result.id);
					writer.create(fname, 24000, 16);
#endif
					break;
				case TTS_RES_VOICE:
					printf("speak %d voice: %d bytes\n", result.id, result.voice->length());
					if (result.voice.get()) {
#if defined(TEST_MP3)
						mp3_write(*result.voice);
#elif defined(HAS_OPUS_CODEC)
						decode_write(*result.voice, writer);
#else
						printf("speak %d voice: %d bytes\n", result.id, result.voice->length());
						writer.write(result.voice->data(), result.voice->length());
#endif
					}
					break;
				case TTS_RES_END:
					printf("speak %d end\n", result.id);
					++count;
#ifdef TEST_MP3
					close_mp3_file();
#else
					writer.close();
#endif
					break;
				case TTS_RES_CANCELLED:
					printf("speak %d cancelled\n", result.id);
					++count;
#ifdef TEST_MP3
					close_mp3_file();
#else
					writer.close();
#endif
					break;
				case TTS_RES_ERROR:
					printf("speak %d error: %d\n", result.id, result.err);
					++count;
#ifdef TEST_MP3
					close_mp3_file();
#else
					writer.close();
#endif
					break;
				}
			}
		});

#if !defined(TEST_MP3) && defined(HAS_OPUS_CODEC)
	rkdecoder.close();
#endif
	// wait thread run
	while (!thflag) {
		// std::this_thread::sleep_for(std::chrono::milliseconds(50));
		usleep(50000);
	}
	// shared_ptr<TtsOptions> tts_opts = TtsOptions::new_instance();
	// tts_opts->set_codec(Codec::OPU2);
	// tts->config(tts_opts);
	int i;
	for (i = 0; i < speakCount; ++i) {
		tts->speak(test_text[i]);
	}
	poll_thread.join();
	tts->release();
}

static void test_speech(PrepareOptions& opts) {
	shared_ptr<Speech> speech = Speech::new_instance();
	int reqCount = sizeof(test_text) / sizeof(char*);
	bool thflag = false;

	speech->prepare(opts);
#ifdef HAS_OPUS_CODEC
	shared_ptr<SpeechOptions> sopts = SpeechOptions::new_instance();
	sopts->set_codec(Codec::OPU);
	speech->config(sopts);
	rkencoder.init(16000, 27800, 20);
#endif
	std::thread poll_thread([&speech, &thflag, reqCount]() {
			int count = 0;
			SpeechResult result;

			thflag = true;
			while (count < reqCount) {
				if (!speech->poll(result))
					break;
				switch (result.type) {
				case SPEECH_RES_START:
					printf("speech %d start\n", result.id);
					break;
				case SPEECH_RES_ASR_FINISH:
					printf("speech %d asr is %s\n", result.id, result.asr.c_str());
					break;
				case SPEECH_RES_END:
					printf("speech %d nlp is %s\n", result.id, result.nlp.c_str());
					printf("speech %d action is %s\n", result.id, result.action.c_str());
					++count;
					break;
				case SPEECH_RES_CANCELLED:
					printf("speech %d cancelled\n", result.id);
					++count;
					break;
				case SPEECH_RES_ERROR:
					printf("speech %d error: code %d\n", result.id, result.err);
					++count;
					break;
				}
			}
		});
	// wait thread run
	while (!thflag) {
		// std::this_thread::sleep_for(std::chrono::milliseconds(50));
		usleep(50000);
	}
	shared_ptr<SpeechOptions> speech_opts = SpeechOptions::new_instance();
	// speech_opts->set_codec(Codec::OPU);
	speech_opts->set_vad_mode(VadMode::LOCAL);
	speech->config(speech_opts);
	int i;
	char fname[32];
	int32_t id;
	SimpleWaveReader reader;
	const void* data;
	uint32_t length;
	uint32_t frame_len;
	uint32_t off;
#ifdef HAS_OPUS_CODEC
	uint32_t frames_size = ENCODE_FRAMES * rkencoder.pcm_frame_size();
	uint32_t total_pcm_frames;
	uint32_t enc_frames, enc_size;
	const uint8_t* opu;
#endif
	for (i = 0; i < reqCount; ++i) {
		snprintf(fname, sizeof(fname), AUDIO_FILE_NAME_FORMAT, i + 1);
		if (reader.open(fname)) {
			data = reader.data();
			length = reader.size();
			printf("%s: data length %u, sample rate %u, bits per sample %u\n",
					fname, length, reader.sample_rate(), reader.bits_per_sample());
			id = speech->start_voice();
			off = 0;
#ifdef HAS_OPUS_CODEC
			total_pcm_frames = length / rkencoder.pcm_frame_size() / sizeof(uint16_t);
			while (off < total_pcm_frames) {
				frame_len = total_pcm_frames - off;
				if (frame_len > ENCODE_FRAMES)
					frame_len = ENCODE_FRAMES;
				opu = rkencoder.encode(reinterpret_cast<const uint16_t*>(data)
						+ off * rkencoder.pcm_frame_size(), frame_len, enc_frames,
						enc_size);
				if (opu == NULL) {
					printf("opu encode failed\n");
					break;
				}
				speech->put_voice(id, opu, enc_size);
				off += enc_frames;
				// 模拟真实拾音发送语音数据的频率
				// std::this_thread::sleep_for(std::chrono::milliseconds(400));
				usleep(100000);
			}
#else
			while (off < length) {
				if (length - off > 2048)
					frame_len = 2048;
				else
					frame_len = length - off;
				speech->put_voice(id, reinterpret_cast<const uint8_t*>(data) + off, frame_len);
				off += frame_len;
				// 模拟真实拾音发送语音数据的频率
				// std::this_thread::sleep_for(std::chrono::milliseconds(400));
				usleep(20000);
			}
#endif
			reader.close();
			speech->end_voice(id);
		}
	}
	poll_thread.join();
	speech->release();
}

int main(int argc, char** argv) {
	PrepareOptions opts;
	opts.host = "apigwws.open.rokid.com";
	opts.port = 443;
	opts.branch = "/api";
	opts.key = "rokid_test_key";
	opts.device_type_id = "rokid_test_device_type_id";
	opts.device_id = "ming.qiwo";
	opts.secret = "rokid_test_secret";

	test_tts(opts);
	test_speech(opts);

	return 0;
}
