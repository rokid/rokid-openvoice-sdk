#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "rlog.h"

using std::string;

namespace rokid {
namespace speech {

typedef struct {
	int fd;
	string path;
	RokidLogEndpoint hook_endpoint;
} FileLogExConfig;

static const char* ENDPOINT_NAME = "file-ex";
static FileLogExConfig config_ = { -1 };

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
			delete h;
			return false;
		}
		++p;
	}
	delete h;
	return true;
}

static void open_log_file(FileLogExConfig* config) {
	if (config->fd >= 0)
		return;
	size_t len = config->path.length();
	assert(len > 0);
	if (config->path.at(len - 1) != '/')
		config->path += '/';
	if (!mkdirs(config->path))
		return;
	struct timeval tv;
	struct tm tm;
	char fname[128];
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tm);
	snprintf(fname, sizeof(fname), "%s%u.%04d%02d%02d%02d%02d%02d.log",
			config->path.c_str(), getpid(), tm.tm_year + 1900, tm.tm_mon,
			tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	config->fd = open(fname, O_CREAT | O_APPEND | O_WRONLY, 0644);
}

static int32_t fileex_log_writer(RokidLogLevel lv, const char* tag,
		const char* fmt, va_list ap, void* arg1, void* arg2) {
	FileLogExConfig* config = (FileLogExConfig*)arg2;

	open_log_file(config);
	if (config->fd < 0)
		return -1;
	// hook builtin endpoint 'file'
	int32_t c = config->hook_endpoint.writer(lv, tag, fmt, ap,
			config->hook_endpoint.arg, (void*)(intptr_t)config->fd);
	// if write to fd failed, set the fd to -1
	// next log write will try reopen the file
	if (c <= 0)
		config->fd = -1;
	return c;
}

void final_file_log() {
	rokid_log_ctl(ROKID_LOG_CTL_DEFAULT_ENDPOINT, "stdout", NULL);
	rokid_log_ctl(ROKID_LOG_CTL_REMOVE_ENDPOINT, ENDPOINT_NAME);
	if (config_.fd >= 0) {
		close(config_.fd);
		config_.fd = -1;
	}
}

void init_file_log(const char* path) {
	int32_t num = rokid_log_endpoints(NULL, 0);
	RokidLogEndpoint* eps = NULL;
	bool found_hook_ep = false;

	if (num) {
		eps = new RokidLogEndpoint[num];
		rokid_log_endpoints(eps, num);
		int32_t i;
		for (i = 0; i < num; ++i) {
			if (strcmp(eps[i].name, ENDPOINT_NAME) == 0) {
				final_file_log();
				continue;
			}
			if (strcmp(eps[i].name, "file") == 0) {
				memcpy(&config_.hook_endpoint, eps + i, sizeof(RokidLogEndpoint));
				found_hook_ep = true;
			}
		}
		delete eps;
	}

	if (!found_hook_ep)
		return;
	rokid_log_ctl(ROKID_LOG_CTL_ADD_ENDPOINT, ENDPOINT_NAME, fileex_log_writer, NULL);
	if (path)
		config_.path = path;
	else
		config_.path = ".";
	open_log_file(&config_);
	rokid_log_ctl(ROKID_LOG_CTL_DEFAULT_ENDPOINT, ENDPOINT_NAME, &config_);
}

void enable_file_log(bool enable, const char* log_path) {
	if (enable)
		init_file_log(log_path);
	else
		final_file_log();
}

} // namespace speech
} // namespace rokid
