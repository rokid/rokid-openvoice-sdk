#include <stdio.h>
#include "tts_mem_test.h"

void tts_demo();

void speech_demo();

int main(int argc, char** argv) {
	// tts_demo();

	// speech_demo();

	TtsMemTest test;
	test.run();
	return 0;
}
