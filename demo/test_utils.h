#pragma once

#include <stdint.h>

uint8_t* read_pcm_data(const char* file, uint32_t& size);

void release_pcm_data(uint8_t* data, uint32_t size);
