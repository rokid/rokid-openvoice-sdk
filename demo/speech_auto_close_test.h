#pragma once

#include "speech.h"
#include "defs.h"
#include "speech_base_test.h"

class SpeechAutoCloseTest : public SpeechBaseTest {
public:
  static void test(const rokid::speech::PrepareOptions& opts, const DemoOptions& dopts);

  void run(const rokid::speech::PrepareOptions& opt, const uint8_t* data, uint32_t size);

private:
  bool requiring = false;
};
