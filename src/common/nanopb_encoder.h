#pragma once

#include <string>
#include <mutex>
#include "auth.pb.h"
#include "tts.pb.h"
#include "speech.pb.h"

namespace rokid {
namespace speech {

class Buffer {
public:
	Buffer(uint32_t size);

	~Buffer();

	void* lock();

	void unlock();

	inline uint32_t size() const { return dataSize; }

private:
	void* data;
	uint32_t dataSize;
	std::mutex _mutex;
};

class NanoPBEncoder {
public:
	bool SerializeToString(std::string* str);

	bool encodeAsSubMsg(pb_ostream_t* stream);

protected:
	void set_string_field(pb_callback_t* cbt, const std::string& str);

private:
	static bool encode_string(pb_ostream_t* stream, const pb_field_t* field, void* const* arg);

	static bool encode_submsg(pb_ostream_t* stream, const pb_field_t* field, void* const* arg);

protected:
	static Buffer encodeBuffer;

	void* nanopbStructPointer;
	const pb_field_t* nanopbFields;
};

class AuthRequest : public NanoPBEncoder {
public:
	AuthRequest();

	~AuthRequest();

	void set_key(const std::string& str);

	void set_device_type_id(const std::string& str);

	void set_device_id(const std::string& str);

	void set_service(const std::string& str);

	void set_version(const std::string& str);

	void set_timestamp(const std::string& str);

	void set_sign(const std::string& str);

private:
	rokid_open_speech_AuthRequest nanopbStruct;
	std::string _key;
	std::string _device_type_id;
	std::string _device_id;
	std::string _service;
	std::string _version;
	std::string _timestamp;
	std::string _sign;
};

class TtsRequest : public NanoPBEncoder {
public:
	TtsRequest();

	void set_id(int32_t i);

	void set_text(const std::string& str);

	void set_declaimer(const std::string& str);

	void set_codec(const std::string& str);

	void set_sample_rate(uint32_t i);

private:
	rokid_open_speech_v1_TtsRequest nanopbStruct;
	std::string _text;
	std::string _declaimer;
	std::string _codec;
};

class SpeechOptionsEnc : public NanoPBEncoder {
public:
	SpeechOptionsEnc(void* st);

	void set_lang(int32_t i);

	void set_codec(int32_t i);

	void set_vad_mode(int32_t i);

	void set_vend_timeout(uint32_t i);

	void set_no_nlp(bool b);

	void set_no_intermediate_asr(bool b);

	void set_stack(const std::string& str);

	void set_voice_trigger(const std::string& str);

	void set_voice_power(float f);

	void set_trigger_start(uint32_t i);

	void set_trigger_length(uint32_t i);

	void set_skill_options(const std::string& str);

	void set_voice_extra(const std::string& str);

private:
	rokid_open_speech_v2_SpeechOptions* nanopbStruct;
	std::string _stack;
	std::string _voice_trigger;
	std::string _skill_options;
	std::string _voice_extra;
};

class SpeechRequest : public NanoPBEncoder {
public:
	SpeechRequest();

	~SpeechRequest();

	void set_id(int32_t i);

	void set_type(int32_t i);

	void set_voice(const std::string& str);

	void set_asr(const std::string& str);

	SpeechOptionsEnc* mutable_options();

private:
	rokid_open_speech_v2_SpeechRequest nanopbStruct;
	std::string _voice;
	std::string _asr;
	SpeechOptionsEnc* _options;
};

class PingPayload : public NanoPBEncoder {
public:
	PingPayload();

	void set_req_id(int32_t i);

	void set_now_tp(uint64_t i);

	void set_req_tp(uint64_t i);

	void set_resp_tp(uint64_t i);

private:
	rokid_open_speech_v1_PingPayload nanopbStruct;
};

} // namespace speech
} // namespace rokid
