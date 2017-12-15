#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "rkaudio.h"

#define MAGIC_NUM 0xe88ba5e7
#define VERSION_NUM 1

using std::string;
using std::shared_ptr;
using std::make_shared;

RKAudioFileWriter::RKAudioFileWriter() : _file(NULL), _cur_file_pos(0) {
	_header.magic = MAGIC_NUM;
	_header.version = VERSION_NUM;
	_header.data_num = 0;
	_header.data_index_section = 0;
}

RKAudioFileWriter::~RKAudioFileWriter() {
	close();
}

bool RKAudioFileWriter::create(const char* fname) {
	if (fname == NULL || _file != NULL)
		return false;
	_file = fopen(fname, "w+");
	if (_file == NULL)
		return false;
	if (fwrite(&_header, sizeof(_header), 1, _file) == 0) {
		fclose(_file);
		_file = NULL;
		return false;
	}
	_cur_file_pos = sizeof(_header);
	_header.data_num = 0;
	return true;
}

bool RKAudioFileWriter::write(const string& data) {
	RKAudioDataIndex idx;
	idx.length = data.length();
	if (fwrite(data.data(), 1, idx.length, _file) != idx.length) {
		fseek(_file, _cur_file_pos, SEEK_SET);
		return false;
	}
	idx.position = _cur_file_pos;
	_data_indexs.push_back(idx);
	_cur_file_pos += idx.length;
	++_header.data_num;
	return true;
}

void RKAudioFileWriter::close() {
	if (_file == NULL)
		return;
	// align data section
	uint32_t index_begin = (_cur_file_pos + 3) & ~3;
	uint32_t pad_count = index_begin - _cur_file_pos;
	RKAudioDataIndex* idxs;
	if (pad_count) {
		uint8_t zero = 0;
		if (fwrite(&zero, 1, pad_count, _file) != pad_count)
			goto exit;
	}
	idxs = _data_indexs.data();
	if (fwrite(idxs, sizeof(RKAudioDataIndex), _data_indexs.size(), _file)
			!= _data_indexs.size())
		goto exit;
	_header.data_index_section = index_begin;
	fseek(_file, 0, SEEK_SET);
	fwrite(&_header, sizeof(_header), 1, _file);

exit:
	_data_indexs.clear();
	fclose(_file);
	_file = NULL;
}

RKAudioFileReader::RKAudioFileReader() : _header(NULL), _data_indexs(NULL),
		_cur_read_index(0), _fd(-1), _file_map(NULL), _file_size(0) {
}

RKAudioFileReader::~RKAudioFileReader() {
	close();
}

bool RKAudioFileReader::open(const char* fname) {
	if (fname == NULL || _fd >= 0)
		return false;
	_fd = ::open(fname, O_RDONLY);
	if (_fd < 0)
		return false;
	int32_t fsz = lseek(_fd, 0, SEEK_END);
	if (fsz < 0) {
		::close(_fd);
		_fd = -1;
		return false;
	}
	_file_map = (char*)mmap(NULL, fsz, PROT_READ, MAP_PRIVATE, _fd, 0);
	if (_file_map == NULL) {
		::close(_fd);
		_fd = -1;
		return false;
	}
	_header = reinterpret_cast<RKAudioFileHeader*>(_file_map);
	int32_t min_idx_pos = sizeof(RKAudioFileHeader);
	int32_t max_idx_pos = fsz - _header->data_num * sizeof(RKAudioDataIndex);
	if (_header->magic != MAGIC_NUM)
		goto failed;
	if (_header->version != VERSION_NUM)
		goto failed;
	if (max_idx_pos < min_idx_pos)
		goto failed;
	if (_header->data_index_section < min_idx_pos || _header->data_index_section > max_idx_pos)
		goto failed;
	if ((_header->data_index_section & 3) != 0)
		goto failed;
	_data_indexs = reinterpret_cast<RKAudioDataIndex*>(_file_map + _header->data_index_section);
	_file_size = fsz;
	_cur_read_index = 0;
	return true;

failed:
	munmap(_file_map, fsz);
	::close(_fd);
	_fd = -1;
	return false;
}

shared_ptr<string> RKAudioFileReader::read() {
	shared_ptr<string> data;
	RKAudioDataIndex* idx;

	if (_fd < 0)
		goto exit;
	if (_cur_read_index >= _header->data_num)
		goto exit;
	idx = _data_indexs + _cur_read_index;
	if (idx->position + idx->length > _header->data_index_section)
		goto exit;
	data = make_shared<string>(_file_map + idx->position, idx->length);
	++_cur_read_index;

exit:
	return data;
}

void RKAudioFileReader::close() {
	if (_fd < 0)
		return;
	munmap(_file_map, _file_size);
	::close(_fd);
	_fd = -1;
}
