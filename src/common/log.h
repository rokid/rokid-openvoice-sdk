#pragma once

#include <stdarg.h>
#include <mutex>

namespace rokid {
namespace speech {

enum LogLevel {
	Verbose,
	Debug,
	Info,
	Warning,
	Error,
	MaxLogLevel
};

class Log {
public:
	static void v(const char* tag, const char* fmt, ...);

	static void d(const char* tag, const char* fmt, ...);

	static void i(const char* tag, const char* fmt, ...);

	static void w(const char* tag, const char* fmt, ...);

	static void e(const char* tag, const char* fmt, ...);

private:
	Log();

	static Log* instance();

	void p(LogLevel level, const char* tag, const char* fmt, va_list ap);

private:
	static Log* instance_;
	std::mutex mutex_;
};

} // namespace speech
} // namespace rokid
