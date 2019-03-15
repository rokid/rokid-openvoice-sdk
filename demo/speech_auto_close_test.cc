#include <unistd.h>
#include <thread>
#include "speech_auto_close_test.h"
#include "rlog.h"

using namespace rokid::speech;
using namespace std;

#define TAG "SpeechBaseTest"

void SpeechAutoCloseTest::run(const PrepareOptions& opt, const uint8_t* data, uint32_t size) {
  shared_ptr<Speech> speech = Speech::new_instance();
  shared_ptr<SpeechOptions> sopts = SpeechOptions::new_instance();
  PrepareOptions popts = opt;
  popts.ping_interval = 4000;
  popts.no_resp_timeout = 20000;
  popts.conn_duration = 10;
  speech->prepare(popts);
  sopts->set_vad_mode(VadMode::CLOUD, 500);
  speech->config(sopts);

  thread poll_thread(
      [this, &speech]() {
        this->speech_poll_routine(speech);
      });

  do_request(0, data, size, speech);
  sleep(15);
  do_request(0, data, size, speech);
  sleep(5);
  do_request(0, data, size, speech);
  sleep(15);
  do_request(0, data, size, speech);

  speech->release();
  poll_thread.join();
}

void SpeechAutoCloseTest::test(const PrepareOptions& opts, const DemoOptions& dopts) {
  SpeechBaseTest::test(opts, dopts);
}
