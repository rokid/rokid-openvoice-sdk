#include "tts_req_provider.h"
#include "tts_impl.h"

using std::string;
using std::shared_ptr;
using grpc::ClientContext;
using rokid::open::TtsRequest;
using rokid::open::TtsHeader;

namespace rokid {
namespace speech {

shared_ptr<TtsReqInfo> TtsReqProvider::poll() {
	int32_t id;
	shared_ptr<string> data;
	bool del;
	bool r = requests_.poll(id, data, del);
	if (!r)
		return NULL;
	shared_ptr<TtsReqInfo> info(new TtsReqInfo());
	info->id = id;
	info->deleted = del;
	info->data = data;
	return info;
}

bool TtsReqProvider::closed() {
	return requests_.closed();
}

} // namespace speech
} // namespace rokid
