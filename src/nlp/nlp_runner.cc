#include <grpc++/client_context.h>
#include "nlp_runner.h"
#include "speech_connection.h"

using grpc::ClientContext;
using grpc::Status;
using rokid::open::NlpRequest;
using rokid::open::NlpHeader;
using rokid::open::NlpResponse;

namespace rokid {
namespace speech {

bool NlpRunner::prepare(SpeechConfig* config) {
	if (!NlpRunnerBase::prepare())
		return false;
	stub_ = SpeechConnection::connect(config, "nlp");
	if (stub_.get() == NULL) {
		NlpRunnerBase::close();
		return false;
	}
	return true;
}

void NlpRunner::close() {
	NlpRunnerBase::close();
}

void NlpRunner::do_request(int32_t id, shared_ptr<string> data) {
	NlpRequest req;
	shared_ptr<NlpResponse> resp(new NlpResponse());
	ClientContext context;
	Status status;

	NlpHeader* header = req.mutable_header();
	header->set_id(id);
	header->set_lang("zh");
	req.set_asr(data->c_str());
	status = stub_->nlp(&context, req, resp.get());
	shared_ptr<NlpRespItem> ri(new NlpRespItem());
	if (status.ok()) {
		ri->type = 0;
		ri->error = 0;
		ri->nlp = resp->nlp();
		responses()->add(id, ri);
	} else {
		ri->type = 2;
		// TODO: set error code
		ri->error = 1;
		responses()->add(id, ri);
	}
}

} // namespace speech
} // namespace rokid
