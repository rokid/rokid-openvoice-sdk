#include <stdio.h>
#include <chrono>
#include "log.h"

#ifdef SPEECH_LOG_ANDROID
#include <android/log.h>
#endif

namespace rokid {
namespace speech {

Log* Log::instance_ = NULL;

Log::Log() {
}

Log* Log::instance() {
	if (instance_ == NULL)
		instance_ = new Log();
	return instance_;
}

void Log::v(const char* tag, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	instance()->p(Verbose, tag, fmt, ap);
	va_end(ap);
}

void Log::d(const char* tag, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	instance()->p(Debug, tag, fmt, ap);
	va_end(ap);
}

void Log::i(const char* tag, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	instance()->p(Info, tag, fmt, ap);
	va_end(ap);
}

void Log::w(const char* tag, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	instance()->p(Warning, tag, fmt, ap);
	va_end(ap);
}

void Log::e(const char* tag, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	instance()->p(Error, tag, fmt, ap);
	va_end(ap);
}

#ifdef SPEECH_LOG_ANDROID
static uint32_t AndroidLogLevels[] = {
	ANDROID_LOG_VERBOSE,
	ANDROID_LOG_DEBUG,
	ANDROID_LOG_INFO,
	ANDROID_LOG_WARN,
	ANDROID_LOG_ERROR
};
#else
static char PosixLogLevels[] = {
	'V',
	'D',
	'I',
	'W',
	'E'
};
#endif

void Log::p(LogLevel level, const char* tag, const char* fmt, va_list ap) {
	if (level < 0 || level >= MaxLogLevel)
		level = Debug;
#ifdef SPEECH_LOG_ANDROID
	__android_log_vprint(AndroidLogLevels[level], tag, fmt, ap);
#else  // posix log
	std::lock_guard<std::mutex> locker(mutex_);
	printf("%c [%s] ", PosixLogLevels[level], tag);
	vprintf(fmt, ap);
	printf("\n");
#endif
}

} // namespace speech
} // namespace rokid
