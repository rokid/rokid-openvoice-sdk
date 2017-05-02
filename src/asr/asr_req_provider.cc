#include "asr_req_provider.h"
#include "asr_impl.h"

using std::string;
using std::shared_ptr;

namespace rokid {
namespace speech {

shared_ptr<AsrReqInfo> AsrReqProvider::poll() {
	uint32_t type;
	uint32_t err;
	int32_t id;
	shared_ptr<string> voice;
	bool r;

	r = requests_.poll(type, err, id, voice);
	if (!r)
		return NULL;
	shared_ptr<AsrReqInfo> req(new AsrReqInfo());
	req->id = id;
	req->type = type;
	req->voice = voice;
	assert(type < 4);
	return req;
}

bool AsrReqProvider::closed() {
	return requests_.closed();
}

} // namespace speech
} // namespace rokid
