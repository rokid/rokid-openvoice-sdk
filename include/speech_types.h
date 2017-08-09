#pragma once

#include <stdint.h>
#include <memory>
#include <string>

namespace rokid {
namespace speech {

class PrepareOptions {
public:
	PrepareOptions();

	PrepareOptions& operator = (const PrepareOptions& options);

	std::string host;
	uint32_t port;
	std::string branch;
	std::string key;
	std::string device_type_id;
	std::string secret;
	std::string device_id;
};

enum class Lang {
	ZH,
	EN
};

enum class Codec {
	PCM,
	OPU,
	OPU2
};

enum class VadMode {
	LOCAL,
	CLOUD
};

} // namespace speech
} // namespace rokid
