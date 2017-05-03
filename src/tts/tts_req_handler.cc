#include <chrono>
#include <grpc++/client_context.h>
#include "speech_connection.h"
#include "log.h"
#include "tts_req_handler.h"
#include "tts_impl.h"

using std::shared_ptr;
using grpc::ClientContext;
using rokid::open::TtsRequest;
using rokid::open::TtsHeader;
using rokid::open::Speech;

namespace rokid {
namespace speech {

const uint32_t grpc_timeout_ = 5;

TtsReqHandler::TtsReqHandler() : cancel_handler_(NULL) {
}

shared_ptr<TtsRespStream> TtsReqHandler::poll() {
	shared_ptr<TtsRespStream> tmp = stream_;
	stream_.reset();
	return tmp;
}

static void config_client_context(ClientContext* ctx) {
	std::chrono::system_clock::time_point deadline =
		std::chrono::system_clock::now() + std::chrono::seconds(grpc_timeout_);
	ctx->set_deadline(deadline);
}

void TtsReqHandler::start_handle(shared_ptr<TtsReqInfo> in, void* arg) {
	if (in.get() && !in->deleted) {
		TtsCommonArgument* carg = (TtsCommonArgument*)arg;
		carg->current_id = in->id;
		carg->context = new ClientContext();
		config_client_context(carg->context);
	}
}

int32_t TtsReqHandler::handle(shared_ptr<TtsReqInfo> in, void* arg) {
	if (!in.get())
		return FLAG_ERROR;
	TtsCommonArgument* carg = (TtsCommonArgument*)arg;
	if (in->deleted) {
		cancel_handler_->cancelled(in->id);
		return FLAG_BREAK_LOOP;
	}

	TtsRequest req;
	TtsHeader* header = req.mutable_header();
	header->set_id(in->id);
	const char* v = carg->config.get("codec", "pcm");
	header->set_codec(v);
	v = carg->config.get("declaimer", "zh");
	header->set_declaimer(v);
	req.set_text(*in->data);
	shared_ptr<Speech::Stub> stub = carg->stub();
	// stub is NULL, config incorrect or auth failed
	// discard this request, try again at next request
	if (stub.get() == NULL)
		return FLAG_BREAK_LOOP;
	stream_ = stub->tts(carg->context, req);
	return 0;
}

void TtsReqHandler::end_handle(shared_ptr<TtsReqInfo> in, void* arg) {
	// do nothing
}

bool TtsReqHandler::closed() {
	return false;
}

} // namespace speech
} // namespace rokid
