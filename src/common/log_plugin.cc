#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "rlog.h"

using std::string;

namespace rokid {
namespace speech {

static bool mkdirs(const string& path);

class FileLogWriter : public RLogWriter {
public:
  bool init(void *arg) {
    string path = reinterpret_cast<char *>(arg);
    if (!open_log_file(path))
      return false;
    return true;
  }

  void destroy() {
    if (fd >= 0) {
      ::close(fd);
      fd = -1;
    }
  }

  bool write(const char *data, uint32_t size) {
    return ::write(fd, data, size) > 0;
  }

private:
  bool open_log_file(string &path) {
    size_t len = path.length();
    if (len == 0)
      return false;
    if (path.at(len - 1) != '/')
      path += '/';
    if (!mkdirs(path))
      return false;
    struct timeval tv;
    struct tm tm;
    char fname[128];
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    snprintf(fname, sizeof(fname), "%s%u.%04d%02d%02d%02d%02d%02d.log",
        path.c_str(), getpid(), tm.tm_year + 1900, tm.tm_mon,
        tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fd = open(fname, O_CREAT | O_APPEND | O_WRONLY, 0644);
    return fd >= 0;
  }

private:
  int fd = -1;
};

static const char* ENDPOINT_NAME = "file-ex";
static FileLogWriter file_log_writer_;

static bool mkdirs(const string& path) {
	char* p;
	char* h = new char[path.length() + 1];
	int r;
	strcpy(h, path.c_str());

	p = h + 1;
	while (*p) {
		if (*p == '/') {
			*p = '\0';
			r = mkdir(h, 0755);
			if (r == 0 || errno == EEXIST) {
				*p = '/';
				++p;
				continue;
			}
			// mkdir failed
			delete[] h;
			return false;
		}
		++p;
	}
	delete[] h;
	return true;
}

static void init_file_log(const char* path) {
  if (RLog::add_endpoint(ENDPOINT_NAME, &file_log_writer_) == 0) {
    RLog::enable_endpoint(ENDPOINT_NAME, (void *)path, true);
  }
}

static void final_file_log() {
  RLog::remove_endpoint(ENDPOINT_NAME);
}

void enable_file_log(bool enable, const char* log_path) {
	if (enable)
		init_file_log(log_path);
	else
		final_file_log();
}

} // namespace speech
} // namespace rokid
