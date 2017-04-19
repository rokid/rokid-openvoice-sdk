#include "grpc++/impl/codegen/proto_utils.h"
#include "asr_resp_handler.h"
#include "speech.pb.h"
#include "log.h"

using std::shared_ptr;
using std::string;
using grpc::Status;
using rokid::open::AsrResponse;

namespace rokid {
namespace speech {

AsrRespHandler::AsrRespHandler() : start_stream_(false) {
}

void AsrRespHandler::start_handle(shared_ptr<AsrRespInfo> in, void* arg) {
	start_stream_ = true;
}

int32_t AsrRespHandler::handle(shared_ptr<AsrRespInfo> in, void* arg) {
	AsrResponse resp;
	CommonArgument* carg = (CommonArgument*)arg;

	assert(arg);
	if (!in.get())
		return FLAG_ERROR;
	if (start_stream_) {
		start_stream_ = false;
		responses_.start(in->id);
		Log::d(tag__, "AsrRespHandler: %d, asr start", in->id);
		return FLAG_NOT_POLL_NEXT;
	}
	if (in->stream->Read(&resp)) {
		string* tmp = resp.release_asr();
		if (tmp) {
			shared_ptr<string> asr(tmp);
			responses_.stream(in->id, asr);
			Log::d(tag__, "AsrRespHandler: %d, read asr %s", in->id, asr->c_str());
			return FLAG_NOT_POLL_NEXT;
		}
		return FLAG_NOT_POLL_NEXT | FLAG_BREAK_LOOP;
	}

	Status status = in->stream->Finish();
	if (status.ok()) {
		responses_.end(in->id);
		Log::d(tag__, "AsrRespHandler: %d end", in->id);
		return 0;
	}
	Log::d(tag__, "AsrRespHandler: grpc status %d:%s", status.error_code(), status.error_message().c_str());
	uint32_t err;
	switch (status.error_code()) {
		case grpc::StatusCode::UNAVAILABLE:
			err = 1;
			break;
		default:
			// undefined error
			err = 2;
			break;
	}
	responses_.erase(in->id, err);
	return 0;
}

void AsrRespHandler::end_handle(shared_ptr<AsrRespInfo> in, void* arg) {
}

shared_ptr<AsrResult> AsrRespHandler::poll() {
	uint32_t err;
	int32_t id;
	shared_ptr<string> asr;
	int32_t r = responses_.pop(id, asr, err);
	if (r < 0)
		return NULL;
	shared_ptr<AsrResult> res(new AsrResult());
	res->id = id;
	res->type = r;
	res->err = err;
	if (r == 0) {
		assert(asr.get());
		res->asr = *asr;
	}
	return res;
}

bool AsrRespHandler::closed() {
	return false;
}

} // namespace speech
} // namespace rokid
