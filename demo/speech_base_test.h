#pragma once

#include "speech.h"
#include "defs.h"

class SpeechBaseTest {
public:
	static void test(const rokid::speech::PrepareOptions& opts, const DemoOptions& dopts);

	void run(const rokid::speech::PrepareOptions& opt, const uint8_t* data, uint32_t size);

private:
	void speech_poll_routine(std::shared_ptr<rokid::speech::Speech>& speech);

private:
	bool working = false;
};
