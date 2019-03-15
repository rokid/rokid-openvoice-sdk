#pragma once

#include "speech.h"
#include "defs.h"

class SpeechBaseTest {
public:
  static void test(const rokid::speech::PrepareOptions& opts, const DemoOptions& dopts);

  virtual void run(const rokid::speech::PrepareOptions& opt, const uint8_t* data, uint32_t size);

protected:
  void do_test(const rokid::speech::PrepareOptions& opts, const DemoOptions& dopts);

  void speech_poll_routine(std::shared_ptr<rokid::speech::Speech>& speech);

  uint32_t do_request(uint32_t off, const uint8_t* data, uint32_t size,
                  std::shared_ptr<rokid::speech::Speech> &speech);

private:
  bool requiring = false;
};
