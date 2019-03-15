#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <thread>
#include <string.h>
#include "tts.h"
#include "speech.h"
#include "simple_wave.h"
#include "speech_stress_test.h"
#include "speech_release_test.h"
#include "speech_base_test.h"
#include "speech_auto_close_test.h"
#include "clargs.h"
#include "defs.h"
#ifdef HAS_OPUS_CODEC
#include <sys/mman.h>
#include "rkcodec.h"
#endif

using namespace rokid::speech;
using std::shared_ptr;
using std::string;

#define AUDIO_FILE_NAME_FORMAT "data%d.wav"

static const char* test_text[] = {
  "若琪，今天天气怎么样",
  "若琪，唱一首歌"
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

static void test_tts(PrepareOptions& opts) {
  shared_ptr<Tts> tts = Tts::new_instance();
  int speakCount = sizeof(test_text) / sizeof(char*);
  int repeat = 1;
  bool thflag = false;

  tts->prepare(opts);
  shared_ptr<TtsOptions> tts_opts = TtsOptions::new_instance();
#if defined(TEST_MP3)
  tts_opts->set_codec(Codec::MP3);
#elif defined(HAS_OPUS_CODEC)
  tts_opts->set_codec(Codec::OPU2);
#else
  tts_opts->set_codec(Codec::PCM);
#endif
  tts_opts->set_samplerate(16000);
  tts->config(tts_opts);
  std::thread poll_thread([&tts, &thflag, speakCount, repeat]() {
      int count = 0;
      TtsResult result;
      char fname[32];
      SimpleWaveWriter writer;

      thflag = true;
#if !defined(TEST_MP3) && defined(HAS_OPUS_CODEC)
      rkdecoder.init(16000, 16000, 20);
#endif
      while (count < speakCount * repeat) {
        if (!tts->poll(result))
          break;
        switch (result.type) {
        case TTS_RES_START:
          printf("speak %d start\n", result.id);
#ifdef TEST_MP3
          create_mp3_file(result.id % speakCount);
#else
          snprintf(fname, sizeof(fname), AUDIO_FILE_NAME_FORMAT, ((result.id - 1) % speakCount) + 1);
          writer.create(fname, 16000, 16);
#endif
          break;
        case TTS_RES_VOICE:
          if (result.voice.get()) {
#if defined(TEST_MP3)
            mp3_write(*result.voice);
#elif defined(HAS_OPUS_CODEC)
            decode_write(*result.voice, writer);
#else
            printf("speak %d voice: %lu bytes\n", result.id, result.voice->length());
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
  while (repeat) {
    for (i = 0; i < speakCount; ++i) {
      tts->speak(test_text[i]);
    }
    --repeat;
  }
  poll_thread.join();
  tts->release();
}

static void test_speech(PrepareOptions& opts) {
  shared_ptr<Speech> speech = Speech::new_instance();
  int reqCount = sizeof(test_text) / sizeof(char*);
  bool thflag = false;

  speech->prepare(opts);
  shared_ptr<SpeechOptions> sopts = SpeechOptions::new_instance();
  sopts->set_codec(Codec::PCM);
  // sopts->set_vad_begin(55);
  // sopts->set_no_trigger_confirm(true);
  speech->config(sopts);
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

  for (i = 0; i < reqCount; ++i) {
    snprintf(fname, sizeof(fname), AUDIO_FILE_NAME_FORMAT, i + 1);
    if (reader.open(fname)) {
      data = reader.data();
      length = reader.size();
      printf("%s: data length %u, sample rate %u, bits per sample %u\n",
          fname, length, reader.sample_rate(), reader.bits_per_sample());
      id = speech->start_voice();
      off = 0;
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
      reader.close();
      speech->end_voice(id);
    }
  }
  poll_thread.join();
  speech->release();
}

static void test_reconn(PrepareOptions& opts) {
  opts.reconn_interval = 2000;
  shared_ptr<Speech> speech = Speech::new_instance();
  speech->prepare(opts);
  printf("press enter key to quit\n");
  getchar();
  speech->release();
}

static void test_file_log(PrepareOptions& opts) {
  enable_file_log(true, ".");
  test_tts(opts);
  enable_file_log(false, NULL);
  test_speech(opts);
}

static void print_cmdline_prompt(const char* progname) {
  static const char* prompt = "USAGE: %s [options]\n"
    "\n"
    "OPTIONS:\n"
    "-t,--test=TESTCASE\n"
    "\tTESTCASE: stress|speech-release|speech-base\n"
    "-p,--pcm=FILE\n"
    "--wav=FILE\n"
    "\n"
    "TESTCASE: speech-release\n"
    "\t--test-duration=DUR   set test duration time(seconds)\n"
    "\t--min-release-interval=T   set minimal release interval time(ms)\n"
    "\t--max-release-interval=T   set maximal release interval time(ms)\n";
  printf(prompt, progname);
}

static void init_demo_options_value(DemoOptions& opts) {
  memset(&opts, 0, sizeof(opts));
  opts.test_duration = 120;
  opts.min_release_interval = 10000;
  opts.max_release_interval = 30000;
}

static bool check_demo_options_valid(const DemoOptions& opts) {
	if (opts.testcase == 0)
		return false;
	if (opts.testcase == 1 && opts.pcm == nullptr)
		return false;
	if (opts.testcase == 3 && opts.pcm == nullptr && opts.wav == nullptr)
		return false;
	if (opts.testcase == 4 && opts.pcm == nullptr && opts.wav == nullptr)
		return false;
	if (opts.testcase > 4)
		return false;
	return true;
}

static bool create_demo_options(clargs_h handle, DemoOptions& opts) {
  const char* key;
  const char* value;
  int32_t r;

  init_demo_options_value(opts);
  while (true) {
    r = clargs_opt_next(handle, &key, &value);
    if (r)
      break;
    if (strcmp(key, "t") == 0 || strcmp(key, "test") == 0) {
      if (value) {
        if (strcmp(value, "stress") == 0) {
          opts.testcase = 1;
        } else if (strcmp(value, "speech-release") == 0) {
          opts.testcase = 2;
        } else if (strcmp(value, "speech-base") == 0) {
          opts.testcase = 3;
        } else if (strcmp(value, "auto-close") == 0) {
          opts.testcase = 4;
        }
      }
    } else if (strcmp(key, "p") == 0 || strcmp(key, "pcm") == 0) {
      opts.pcm = value;
    } else if (strcmp(key, "wav") == 0) {
      opts.wav = value;
    } else if (strcmp(key, "test-duration") == 0) {
      opts.test_duration = atoi(value);
    } else if (strcmp(key, "min-release-interval") == 0) {
      opts.min_release_interval = atoi(value);
    } else if (strcmp(key, "max-release-interval") == 0) {
      opts.max_release_interval = atoi(value);
    }
  }

  // check valid
  if (!check_demo_options_valid(opts))
    return false;
  return true;
}

int main(int argc, char** argv) {
  clargs_h h = clargs_parse(argc, argv);
  DemoOptions demo_options;
  PrepareOptions opts;

  if (h == 0) {
    goto failed;
  }
  if (!create_demo_options(h, demo_options)) {
    clargs_destroy(h);
    goto failed;
  }

  opts.host = "apigwws.open.rokid.com";
  opts.port = 443;
  opts.branch = "/api";
  opts.key = "6DDECE40ED024837AC9BDC4039DC3245";
  opts.device_type_id = "B16B2DFB5A004DCBAFD0C0291C211CE1";
  opts.device_id = "ming.demo";
  opts.secret = "F2A1FDC667A042F3A44E516282C3E1D7";

  switch (demo_options.testcase) {
    case 1: // stress test
      SpeechStressTest::test(opts, demo_options.pcm, 1);
      break;
    case 2: // speech release test
      SpeechReleaseTest::test(opts, demo_options);
      break;
    case 3:
      SpeechBaseTest::test(opts, demo_options);
      break;
    case 4:
      SpeechAutoCloseTest::test(opts, demo_options);
      break;
  }

  clargs_destroy(h);
  return 0;

failed:
  print_cmdline_prompt(argv[0]);
  return 1;
}
