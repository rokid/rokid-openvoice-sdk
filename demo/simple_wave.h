#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct {
	char id[4];
	uint32_t size;
	char format[4];
} RIFFChunk;

typedef struct {
	char id[4];
	uint32_t size;
	uint16_t audio_format;
	uint16_t channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
} RIFFSubChunk1;

typedef struct {
	char id[4];
	uint32_t size;
} RIFFSubChunk2;

class SimpleWaveWriter {
public:
	SimpleWaveWriter();

	~SimpleWaveWriter();

	bool create(const char* fname, uint32_t sample_rate, uint16_t bits_per_sample);

	void write(const void* data, uint32_t length);

	void close();

private:
	RIFFChunk _chunk;
	RIFFSubChunk1 _sub_chunk1;
	RIFFSubChunk2 _sub_chunk2;
	uint32_t _data_size;
	FILE* _file;
};

class SimpleWaveReader {
public:
	SimpleWaveReader();

	~SimpleWaveReader();

	bool open(const char* fname);

	void close();

	const void* data() const;

	uint32_t size() const;

	uint32_t sample_rate() const;

	uint16_t bits_per_sample() const;

private:
	RIFFChunk* _chunk;
	RIFFSubChunk1* _sub_chunk1;
	RIFFSubChunk2* _sub_chunk2;
	int _fd;
	void* _file_map;
	uint32_t _file_size;
};
