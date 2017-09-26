#pragma once

#include "tts.h"

using namespace std;
using namespace rokid::speech;

class TtsMemTest {
public:
	void run();

private:
	void tts_poll();

private:
	shared_ptr<Tts> _tts;
};
