#include <memory>
#include "nlp_impl.h"
#include "log.h"
#include "speech.grpc.pb.h"
#include "speech_connection.h"

using std::string;
using std::shared_ptr;
using std::unique_ptr;
using rokid::open::Speech;

namespace rokid {
namespace speech {

NlpImpl::NlpImpl() : Pipeline(tag__), requests_(req_provider_.queue()), next_id_(0) {
}

bool NlpImpl::prepare() {
	unique_ptr<Speech::Stub> stub =
		std::move(SpeechConnection::connect(&pipeline_arg_.config, "nlp"));
	if (stub.get() == NULL)
		return false;
	req_handler_.set_grpc_stub(stub);
	req_handler_.set_cancel_handler(&cancel_handler_);
	set_head(&req_provider_);
	add(&req_handler_, &pipeline_arg_);
	add(&cancel_handler_, NULL);
	add_worker(0);
	run();
}

void NlpImpl::release() {
	if (!closed()) {
		requests_->close();
		cancel_handler_.close();
		close();
	}
}

int32_t NlpImpl::request(const char* asr) {
	if (asr == NULL)
		return -1;
	int32_t id = next_id();
	shared_ptr<string> str(new string(asr));
	Log::d(tag__, "NlpImpl: request %d, %s", id, asr);
	if (!requests_->add(id, str))
		return -1;
	return id;
}

void NlpImpl::cancel(int32_t id) {
	Log::d(tag__, "NlpImpl: cancel %d", id);
	if (id > 0) {
		if (!requests_->erase(id))
			cancel_handler_.cancel(id);
	} else {
		int32_t min_id;
		requests_->clear(&min_id, NULL);
		if (min_id > 1)
			// try cancel previous polled request
			cancel_handler_.cancel(min_id - 1);
	}
}

void NlpImpl::config(const char* key, const char* value) {
	pipeline_arg_.config.set(key, value);
}

bool NlpImpl::poll(NlpResult& res) {
	shared_ptr<NlpResult> tmp = cancel_handler_.poll();
	if (tmp.get() == NULL)
		return false;
	res = *tmp;
	return true;
}

Nlp* new_nlp() {
	return new NlpImpl();
}

void delete_nlp(Nlp* nlp) {
	if (nlp)
		delete nlp;
}

} // namespace speech
} // namespace rokid
