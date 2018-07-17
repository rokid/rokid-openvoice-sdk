#pragma once

#include <thread>
#include "defs.h"
#include "speech.h"

using rokid::speech::PrepareOptions;
using rokid::speech::Speech;

class SpeechReleaseTest {
public:
	static void test(const PrepareOptions& opts, const DemoOptions& dopts);

	SpeechReleaseTest(const PrepareOptions& opts, const uint8_t* p, uint32_t size);

	void run(const DemoOptions& dopts);

private:
	void create_work_threads();

	void destroy_work_threads();

	void do_speech_putvoice();

	void do_speech_poll();

private:
	const PrepareOptions* prepare_options;
	const uint8_t* pcm;
	uint32_t pcm_size;
	// 0: speech not prepared
	// 1: speech prepared
	// 2: test finish
	int32_t flags;
	int32_t speech_id;
	std::shared_ptr<Speech> speech;
	std::thread* put_thread;
	std::thread* poll_thread;
};
