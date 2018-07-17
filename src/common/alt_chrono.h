#pragma once

#include <chrono>

#ifdef NO_STD_STEADY_CLOCK

namespace rokid {
namespace altstd {

class steady_clock {
public:
	typedef std::chrono::nanoseconds duration;
	typedef duration::rep rep;
	typedef duration::period period;
	typedef std::chrono::time_point<steady_clock, duration> time_point;

	static time_point now();
};

}
}

typedef rokid::altstd::steady_clock SteadyClock;

#else // NO_STD_STEADY_CLOCK

typedef std::chrono::steady_clock SteadyClock;

#endif // NO_STD_STEADY_CLOCK
