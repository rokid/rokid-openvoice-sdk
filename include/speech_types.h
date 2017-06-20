#pragma once

#include <memory>
#include <string>

namespace rokid {
namespace speech {

class Options {
public:
	virtual ~Options() {}

	virtual bool set(const char* key, const char* value) = 0;

	virtual void clear() = 0;

	virtual void to_json_string(std::string& json) = 0;
};

std::shared_ptr<Options> new_options();

} // namespace speech
} // namespace rokid
