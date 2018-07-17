#pragma once

#include <stdint.h>

typedef struct {
	// common
	uint32_t testcase;
	const char* pcm;
	const char* wav;

	// testcase: speech-release
	uint32_t test_duration; // seconds
	uint32_t min_release_interval; // milliseconds
	uint32_t max_release_interval; // milliseconds
} DemoOptions;

