#pragma once

#include <memory>
#include <string>
#include "speech.grpc.pb.h"
#include "speech_config.h"

using std::string;
using std::unique_ptr;
using std::shared_ptr;

namespace rokid {
namespace speech {

class SpeechConnection {
public:
	static unique_ptr<rokid::open::Speech::Stub> connect(SpeechConfig* config, const char* svc);

private:
	static bool auth(rokid::open::Speech::Stub* stub, SpeechConfig* config, const char* svc);

	static string timestamp();

	static string generate_sign(const char* key, const char* devtype,
			const char* devid, const char* svc, const char* version,
			const char* ts, const char* secret);

	SpeechConnection();
};

} // namespace speech
} // namespace rokid
