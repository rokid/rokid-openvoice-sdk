#include "nlp_req_handler.h"
#include "speech.pb.h"
#include "grpc++/impl/codegen/client_context.h"
#include "grpc++/impl/codegen/proto_utils.h"
#include "log.h"

using std::shared_ptr;
using std::unique_ptr;
using rokid::open::Speech;
using rokid::open::NlpRequest;
using rokid::open::NlpResponse;
using rokid::open::NlpHeader;
using grpc::ClientContext;
using grpc::Status;

namespace rokid {
namespace speech {

static uint32_t grpc_timeout_ = 5;

NlpReqHandler::NlpReqHandler() : cancel_handler_(NULL) {
}

void NlpReqHandler::close() {
	stub_.reset();
}

bool NlpReqHandler::closed() {
	return false;
}

void NlpReqHandler::set_grpc_stub(unique_ptr<Speech::Stub>& stub) {
	stub_ = std::move(stub);
}

static void config_client_context(ClientContext* ctx) {
	std::chrono::system_clock::time_point deadline =
		std::chrono::system_clock::now() + std::chrono::seconds(grpc_timeout_);
	ctx->set_deadline(deadline);
}

void NlpReqHandler::start_handle(shared_ptr<NlpReqInfo> in, void* arg) {
}

int32_t NlpReqHandler::handle(shared_ptr<NlpReqInfo> in, void* arg) {
	NlpRequest req;
	NlpResponse resp;
	NlpHeader* header;
	CommonArgument* carg = (CommonArgument*)arg;

	assert(arg);

	if (!in.get())
		return FLAG_ERROR;

	if (in->deleted) {
		cancel_handler_->cancelled(in->id);
		return FLAG_BREAK_LOOP;
	}

	ClientContext ctx;
	config_client_context(&ctx);
	header = req.mutable_header();
	header->set_id(in->id);
	header->set_lang(carg->config.get("lang", "zh"));
	header->set_cdomain(carg->config.get("cdomain", ""));
	req.set_asr(*in->text);
	Status status = stub_->nlp(&ctx, req, &resp);

	shared_ptr<NlpResult> r(new NlpResult());
	if (status.ok()) {
		Log::d(tag__, "nlp req handle ok %d, %s",
				in->id, in->text->c_str());
		r->id = in->id;
		r->type = 0;
		r->nlp = resp.nlp();
		r->err = 0;
	} else {
		Log::d(tag__, "nlp req handle error: %d, %s",
				status.error_code(), status.error_message().c_str());
		r->id = in->id;
		r->type = 2;
		if (status.error_code() == grpc::StatusCode::UNAVAILABLE)
			r->err = 1;
		else
			r->err = 2;
	}
	responses_.push_back(r);
	return 0;
}

void NlpReqHandler::end_handle(shared_ptr<NlpReqInfo> in, void* arg) {
}

shared_ptr<NlpResult> NlpReqHandler::poll() {
	if (responses_.empty())
		return NULL;
	shared_ptr<NlpResult> res = responses_.front();
	responses_.pop_front();
	return res;
}

} // namespace speech
} // namespace rokid
