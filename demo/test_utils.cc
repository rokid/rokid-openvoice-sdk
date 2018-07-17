#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "test_utils.h"

uint8_t* read_pcm_data(const char* file, uint32_t& size) {
	int fd = open(file, O_RDONLY);
	if (fd < 0)
		return nullptr;
	off_t o = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	uint8_t* d = (uint8_t*)mmap(nullptr, o, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (d)
		size = o;
	return d;
}

void release_pcm_data(uint8_t* data, uint32_t size) {
	munmap(data, size);
}
