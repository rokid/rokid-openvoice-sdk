#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "simple_wave.h"

SimpleWaveWriter::SimpleWaveWriter() : _data_size(0), _file(NULL) {
	_chunk.id[0] = 'R';
	_chunk.id[1] = 'I';
	_chunk.id[2] = 'F';
	_chunk.id[3] = 'F';
	_chunk.size = 0;
	_chunk.format[0] = 'W';
	_chunk.format[1] = 'A';
	_chunk.format[2] = 'V';
	_chunk.format[3] = 'E';
	_sub_chunk1.id[0] = 'f';
	_sub_chunk1.id[1] = 'm';
	_sub_chunk1.id[2] = 't';
	_sub_chunk1.id[3] = ' ';
	_sub_chunk1.size = 16;
	_sub_chunk1.audio_format = 1; // audio format is PCM
	_sub_chunk1.channels = 1;
	_sub_chunk1.sample_rate = 0;
	_sub_chunk1.byte_rate = 0;
	_sub_chunk1.block_align = 0;
	_sub_chunk1.bits_per_sample = 0;
	_sub_chunk2.id[0] = 'd';
	_sub_chunk2.id[1] = 'a';
	_sub_chunk2.id[2] = 't';
	_sub_chunk2.id[3] = 'a';
	_sub_chunk2.size = 0;
}

SimpleWaveWriter::~SimpleWaveWriter() {
	close();
}

bool SimpleWaveWriter::create(const char* fname, uint32_t sample_rate, uint16_t bits_per_sample) {
	if (fname == NULL || _file)
		return false;
	_file = fopen(fname, "w+");
	if (_file == NULL)
		return false;
	_sub_chunk1.sample_rate = sample_rate;
	_sub_chunk1.byte_rate = sample_rate * bits_per_sample / 8;
	_sub_chunk1.block_align = bits_per_sample / 8;
	_sub_chunk1.bits_per_sample = bits_per_sample;
	fwrite(&_chunk, sizeof(_chunk), 1, _file);
	fwrite(&_sub_chunk1, sizeof(_sub_chunk1), 1, _file);
	fwrite(&_sub_chunk2, sizeof(_sub_chunk2), 1, _file);
	long sz = ftell(_file);
	if (sz != sizeof(_chunk) + sizeof(_sub_chunk1) + sizeof(_sub_chunk2)) {
		fclose(_file);
		_file = NULL;
		return false;
	}
	_data_size = 0;
	return true;
}

void SimpleWaveWriter::write(const void* data, uint32_t length) {
	if (_file) {
		size_t sz = fwrite(data, 1, length, _file);
		_data_size += sz;
	}
}

void SimpleWaveWriter::close() {
	if (_file == NULL)
		return;
	_sub_chunk2.size = _data_size;
	_chunk.size = _data_size + 36;
	fseek(_file, 4, SEEK_SET);
	fwrite(&_chunk.size, 4, 1, _file);
	fseek(_file, 40, SEEK_SET);
	fwrite(&_sub_chunk2.size, 4, 1, _file);
	fclose(_file);
	_file = NULL;
}

SimpleWaveReader::SimpleWaveReader() : _chunk(NULL), _sub_chunk1(NULL),
		_sub_chunk2(NULL), _fd(-1), _file_map(NULL), _file_size(0) {
}

SimpleWaveReader::~SimpleWaveReader() {
	close();
}

bool SimpleWaveReader::open(const char* fname) {
	if (fname == NULL || _fd >= 0)
		return false;
	_fd = ::open(fname, O_RDONLY);
	if (_fd < 0)
		return false;
	off_t sz = lseek(_fd, 0, SEEK_END);
	if (sz < sizeof(RIFFChunk) + sizeof(RIFFSubChunk1) + sizeof(RIFFSubChunk2)) {
		::close(_fd);
		_fd = -1;
		return false;
	}
	_file_map = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, _fd, 0);
	if (_file_map == NULL) {
		::close(_fd);
		_fd = -1;
		return false;
	}
	_file_size = sz;
	_chunk = reinterpret_cast<RIFFChunk*>(_file_map);
	_sub_chunk1 = reinterpret_cast<RIFFSubChunk1*>(_chunk + 1);
	_sub_chunk2 = reinterpret_cast<RIFFSubChunk2*>(_sub_chunk1 + 1);
	return true;
}

void SimpleWaveReader::close() {
	if (_fd >= 0) {
		munmap(_file_map, _file_size);
		::close(_fd);
		_fd = -1;
	}
}

const void* SimpleWaveReader::data() const {
	if (_fd < 0)
		return NULL;
	return reinterpret_cast<void*>(_sub_chunk2 + 1);
}

uint32_t SimpleWaveReader::size() const {
	if (_fd < 0)
		return 0;
	return _sub_chunk2->size;
}

uint32_t SimpleWaveReader::sample_rate() const {
	if (_fd < 0)
		return 0;
	return _sub_chunk1->sample_rate;
}

uint16_t SimpleWaveReader::bits_per_sample() const {
	if (_fd < 0)
		return 0;
	return _sub_chunk1->bits_per_sample;
}
