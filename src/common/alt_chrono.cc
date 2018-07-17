#include "alt_chrono.h"

#ifdef NO_STD_STEADY_CLOCK

using std::chrono::microseconds;
// using std::chrono::seconds;
using std::chrono::duration;

namespace rokid {
namespace altstd {

steady_clock::time_point steady_clock::now() {
	struct timespec cts;
	uint64_t v;
	clock_gettime(CLOCK_MONOTONIC, &cts);
	v = cts.tv_sec;
	v *= 1000000;
	v += cts.tv_nsec / 1000;
//	duration dur = std::chrono::duration_cast<seconds>(nanoseconds(v));
	std::chrono::system_clock::duration dur = microseconds(v);
	return time_point(dur);
}

}
}

#endif // NO_STD_STEADY_CLOCK
