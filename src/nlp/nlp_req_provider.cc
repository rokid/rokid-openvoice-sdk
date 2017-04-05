#include "nlp_req_provider.h"

using std::shared_ptr;
using std::string;

namespace rokid {
namespace speech {

shared_ptr<NlpReqInfo> NlpReqProvider::poll() {
	int32_t id;
	shared_ptr<string> text;
	bool del;

	if (!requests_.poll(id, text, del))
		return NULL;
	Log::d(tag__, "NlpReqProvider: %d, %s", id, text->c_str());
	shared_ptr<NlpReqInfo> r(new NlpReqInfo());
	r->id = id;
	r->text = text;
	r->deleted = del;
	return r;
}

bool NlpReqProvider::closed() {
	return requests_.closed();
}

} // namespace speech
} // namespace rokid
