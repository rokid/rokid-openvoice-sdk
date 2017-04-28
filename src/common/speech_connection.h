#pragma once

#include <memory>
#include <string>
#include "speech.grpc.pb.h"
#include "speech_config.h"

namespace rokid {
namespace speech {

class SpeechConnection {
public:
	static std::shared_ptr<rokid::open::Speech::Stub> connect(SpeechConfig* config, const char* svc);

private:
	static bool auth(rokid::open::Speech::Stub* stub, SpeechConfig* config, const char* svc);

	static std::string timestamp();

	static std::string generate_sign(const char* key, const char* devtype,
			const char* devid, const char* svc, const char* version,
			const char* ts, const char* secret);

	SpeechConnection();
};

} // namespace speech
} // namespace rokid
