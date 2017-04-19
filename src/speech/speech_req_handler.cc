#include <chrono>
#include "speech_req_handler.h"
#include "speech.pb.h"
#include "grpc++/impl/codegen/client_context.h"
#include "log.h"

using std::shared_ptr;
using std::unique_ptr;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using rokid::open::SpeechHeader;
using rokid::open::SpeechResponse;
using rokid::open::TextSpeechRequest;
using rokid::open::VoiceSpeechRequest;
using grpc::Status;
using grpc::ClientContext;

namespace rokid {
namespace speech {

static const uint32_t grpc_timeout_ = 5;

SpeechReqHandler::SpeechReqHandler() : closed_(false) {
}

void SpeechReqHandler::start_handle(shared_ptr<SpeechReqInfo> in, void* arg) {
}

static void prepare_header(SpeechHeader* header, int32_t id, const SpeechConfig& config) {
	header->set_id(id);
	header->set_lang(config.get("lang", "zh"));
	header->set_codec(config.get("codec", "pcm"));
	header->set_vt(config.get("vt", "zh"));
	header->set_cdomain(config.get("cdomain", ""));
}

static void config_client_context(ClientContext* ctx) {
	std::chrono::system_clock::time_point deadline =
		std::chrono::system_clock::now() + std::chrono::seconds(grpc_timeout_);
	ctx->set_deadline(deadline);
}

int32_t SpeechReqHandler::handle(shared_ptr<SpeechReqInfo> in, void* arg) {
	assert(arg);

	Log::d(tag__, "SpeechReqHandler.handle: arg = %p, in = %p",
			arg, in.get());
	if (in.get() == NULL)
		return FLAG_BREAK_LOOP;
	CommonArgument* carg = (CommonArgument*)arg;
	switch (in->type) {
		case 0: {
			TextSpeechRequest req;
			ClientContext ctx;
			SpeechResponse resp;
			shared_ptr<SpeechRespInfo> resp_info(new SpeechRespInfo());

			config_client_context(&ctx);
			prepare_header(req.mutable_header(), in->id, carg->config);
			req.set_asr(*in->data);
			Status status = stub_->speecht(&ctx, req, &resp);
			resp_info->type = 0;
			resp_info->result.id = in->id;
			if (status.ok()) {
				resp_info->result.type = 0;
				resp_info->result.err = 0;
				resp_info->result.asr = resp.asr();
				resp_info->result.nlp = resp.nlp();
				resp_info->result.action = resp.action();
				Log::d(tag__, "call speecht sucess, %d\n\tasr: %s\n\tnlp: %s\n\taction: %s",
						in->id, resp.asr().c_str(),
						resp.nlp().c_str(), resp.action().c_str());
			} else {
				Log::d(tag__, "call speecht failed, %d, %s",
						status.error_code(), status.error_message().c_str());
				resp_info->result.type = 4;
				if (status.error_code() == grpc::StatusCode::UNAVAILABLE)
					resp_info->result.err = 1;
				else
					resp_info->result.err = 2;
			}
			lock_guard<mutex> locker(mutex_);
			responses_.push_back(resp_info);
			cond_.notify_one();
			break;
		}
		case 1: {
			shared_ptr<SpeechRespInfo> resp_info(new SpeechRespInfo());
			VoiceSpeechRequest req;
			shared_ptr<ClientContext> ctx(new ClientContext);

			config_client_context(ctx.get());
			prepare_header(req.mutable_header(), in->id, carg->config);
			resp_info->type = 1;
			Log::d(tag__, "call speechv for %d", in->id);
			resp_info->stream = stub_->speechv(ctx.get());
			carg->stream = resp_info->stream;
			carg->stream->Write(req);
			resp_info->id = in->id;
			resp_info->context = ctx;
			lock_guard<mutex> locker(mutex_);
			responses_.push_back(resp_info);
			cond_.notify_one();
			break;
		}
		case 2: {
			Log::d(tag__, "call WritesDone for %d", in->id);
			carg->stream->WritesDone();
			carg->stream.reset();
			break;
		}
		case 3: {
			Log::d(tag__, "ReqHandler: cancel %d", in->id);
			cancel_handler_->cancel(in->id);
			if (carg->stream.get()) {
				carg->stream->WritesDone();
				carg->stream.reset();
			}
			cancel_handler_->cancelled(in->id);
			break;
		}
		case 4: {
			assert(carg->stream.get());
			Log::d(tag__, "ReqHandler: %d  send voice data %u bytes",
					in->id, in->data->length());
			VoiceSpeechRequest req;
			req.set_voice(*in->data);
			carg->stream->Write(req);
			break;
		}
		default: {
			assert(false);
		}
	}
	return 0;
}

void SpeechReqHandler::end_handle(shared_ptr<SpeechReqInfo> in, void* arg) {
}

shared_ptr<SpeechRespInfo> SpeechReqHandler::poll() {
	unique_lock<mutex> locker(mutex_);

	while(!closed_) {
		if (!responses_.empty()) {
			shared_ptr<SpeechRespInfo> tmp = responses_.front();
			responses_.pop_front();
			Log::d(tag__, "req handler poll %p", tmp.get());
			return tmp;
		}
		cond_.wait(locker);
	}
	Log::d(tag__, "req handler poll null, closed %d", closed_);
	return NULL;
}

void SpeechReqHandler::close() {
	lock_guard<mutex> locker(mutex_);
	closed_ = true;
	cond_.notify_one();
}

bool SpeechReqHandler::closed() {
	return closed_;
}

} // namespace speech
} // namespace rokid
