#pragma once

#include <stdint.h>
#include <string>

namespace rokid {
namespace speech {

typedef struct {
	int32_t id;
	uint32_t err;
	// 0: nlp
	// 1: cancelled nlp request
	// 2: failed nlp request, 'error' is error code
	uint32_t type;
	std::string nlp;
} NlpResult;

class Nlp {
public:
	virtual ~Nlp() {}

	virtual bool prepare() = 0;

	virtual void release() = 0;

	virtual int request(const char* asr) = 0;

	virtual void cancel(int32_t id) = 0;

	virtual void config(const char* key, const char* value) = 0;

	virtual bool poll(NlpResult& res) = 0;
};

Nlp* new_nlp();

void delete_nlp(Nlp* nlp);

} // namespace speech
} // namespace rokid
