#include <assert.h>
#include <chrono>
#include "speech.pb.h"
#include "asr_req_handler.h"
#include "log.h"

using std::shared_ptr;
using std::mutex;
using std::lock_guard;
using std::unique_lock;
using grpc::ClientContext;
using rokid::open::AsrRequest;
using rokid::open::AsrHeader;

namespace rokid {
namespace speech {

static uint32_t grpc_timeout_ = 5;

AsrReqHandler::AsrReqHandler() : closed_(false) {
}

shared_ptr<AsrRespInfo> AsrReqHandler::poll() {
	unique_lock<mutex> locker(mutex_);
	if (resp_streams_.empty()) {
		cond_.wait(locker);
		if (closed_)
			return NULL;
		assert(!resp_streams_.empty());
	}
	shared_ptr<AsrRespInfo> stream = resp_streams_.front();
	assert(stream.get());
	resp_streams_.pop_front();
	return stream;
}

void AsrReqHandler::close() {
	lock_guard<mutex> locker(mutex_);
	closed_ = true;
	cond_.notify_one();
}

bool AsrReqHandler::closed() {
	return closed_;
}

static void config_client_context(ClientContext* ctx) {
	std::chrono::system_clock::time_point deadline =
		std::chrono::system_clock::now() + std::chrono::seconds(grpc_timeout_);
	ctx->set_deadline(deadline);
}

void AsrReqHandler::start_handle(shared_ptr<AsrReqInfo> in, void* arg) {
}

int32_t AsrReqHandler::handle(shared_ptr<AsrReqInfo> in, void* arg) {
	CommonArgument* carg = (CommonArgument*)arg;
	AsrClientStreamSp stream;
	AsrRequest req;
	AsrHeader* header;

	assert(arg);
	if (!in.get())
		return FLAG_ERROR;
	switch (in->type) {
		case 0:
			stream = carg->stream;
			if (stream.get()) {
				req.set_voice(*in->voice);
				if (!stream->Write(req)) {
					// stream broken
					error_code_ = -1;
					return FLAG_ERROR;
				}
			}
			Log::d(tag__, "AsrReqHandler: send voice %u bytes, stream is %p",
					in->voice->length(), stream.get());
			break;
		case 1: {
			shared_ptr<ClientContext> ctx(new ClientContext());
			config_client_context(ctx.get());
			stream = stub_->asr(ctx.get());
			assert(stream.get());
			header = req.mutable_header();
			header->set_id(in->id);
			header->set_lang(carg->config.get("lang", "zh"));
			header->set_codec(carg->config.get("codec", "pcm"));
			header->set_vt(carg->config.get("vt", ""));
			if (!stream->Write(req)) {
				// stream broken
				error_code_ = -1;
				Log::d(tag__, "AsrReqHandler: asr begin, id is %d, stream broken",
						in->id);
				return FLAG_ERROR;
			}
			carg->stream = stream;
			Log::d(tag__, "AsrReqHandler: asr begin, id is %d, stream is %p",
					in->id, stream.get());
			shared_ptr<AsrRespInfo> resp_info(new AsrRespInfo());
			resp_info->id = in->id;
			resp_info->stream = stream;
			resp_info->context = ctx;
			unique_lock<mutex> locker(mutex_);
			resp_streams_.push_back(resp_info);
			cond_.notify_one();
			locker.unlock();
			break;
		}
		case 2:
			stream = carg->stream;
			if (stream.get()) {
				stream->WritesDone();
				carg->stream.reset();
			}
			Log::d(tag__, "AsrReqHandler: asr end, id is %d, stream is %p",
					in->id, stream.get());
			break;
		case 3:
			cancel_handler_->cancel(in->id);
			stream = carg->stream;
			if (stream.get()) {
				stream->WritesDone();
				carg->stream.reset();
			}
			cancel_handler_->cancelled(in->id);
			Log::d(tag__, "AsrReqHandler: asr cancel, id is %d", in->id);
			return FLAG_BREAK_LOOP;
		default:
			// unkown error, impossible happens
			error_code_ = -2;
			Log::d(tag__, "AsrReqHandler: asr unknow error, req type is %u",
					in->type);
			return FLAG_ERROR;
	}
	return 0;
}

void AsrReqHandler::end_handle(shared_ptr<AsrReqInfo> in, void* arg) {
	// do nothing
}

} // namespace speech
} // namespace rokid
