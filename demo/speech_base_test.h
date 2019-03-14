#pragma once

#include "speech.h"
#include "defs.h"

class SpeechBaseTest {
public:
	static void test(const rokid::speech::PrepareOptions& opts, const DemoOptions& dopts);

	void run(const rokid::speech::PrepareOptions& opt, const uint8_t* data, uint32_t size);

protected:
	void speech_poll_routine(std::shared_ptr<rokid::speech::Speech>& speech);

  void do_request(uint32_t off, const uint8_t* data, uint32_t size,
                  std::shared_ptr<rokid::speech::Speech> &speech);

private:
	bool requiring = false;
};
