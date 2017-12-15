#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <memory>

typedef struct {
	uint32_t magic;
	uint32_t version;
	uint32_t data_num;
	uint32_t data_index_section;
} RKAudioFileHeader;

typedef struct {
	uint32_t position;
	uint32_t length;
} RKAudioDataIndex;

class RKAudioFileWriter {
public:
	RKAudioFileWriter();

	~RKAudioFileWriter();

	bool create(const char* fname);

	bool write(const std::string& data);

	void close();

private:
	FILE* _file;
	uint32_t _cur_file_pos;
	RKAudioFileHeader _header;
	std::vector<RKAudioDataIndex> _data_indexs;
};

class RKAudioFileReader {
public:
	RKAudioFileReader();

	~RKAudioFileReader();

	bool open(const char* fname);

	std::shared_ptr<std::string> read();

	void close();

private:
	RKAudioFileHeader* _header;
	RKAudioDataIndex* _data_indexs;
	uint32_t _cur_read_index;
	int _fd;
	char* _file_map;
	int32_t _file_size;
};
