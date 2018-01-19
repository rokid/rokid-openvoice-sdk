#pragma once

#include <string>
#include <memory>
#include "auth.pb.h"
#include "tts.pb.h"
#include "speech.pb.h"

namespace rokid {
namespace speech {

class NanoPBDecoder {
public:
	bool ParseFromArray(const char* data, uint32_t length);

protected:
	void init_string_field(pb_callback_t* cb, std::shared_ptr<std::string>* strp);

	virtual void clear_super_data() {}

private:
	static bool decode_string(pb_istream_t *stream, const pb_field_t *field,
			void **arg);

protected:
	void* nanopbStructPointer;
	const pb_field_t* nanopbFields;
};

class AuthResponse : public NanoPBDecoder {
public:
	AuthResponse();

	inline int32_t result() const {
		return nanopbStruct.result;
	}

private:
	rokid_open_speech_AuthResponse nanopbStruct;
};

class TtsResponse : public NanoPBDecoder {
public:
	TtsResponse();

	inline int32_t id() const {
		return nanopbStruct.id;
	}

	inline int32_t result() const {
		return nanopbStruct.result;
	}

	inline bool has_voice() const {
		return _voice.get() != NULL;
	}

	std::string* release_voice();

	inline bool finish() const {
		return nanopbStruct.finish;
	}

protected:
	void clear_super_data();

private:
	rokid_open_speech_v1_TtsResponse nanopbStruct;
	std::shared_ptr<std::string> _text;
	std::shared_ptr<std::string> _voice;
};

class SpeechResponse : public NanoPBDecoder {
public:
	SpeechResponse();

	inline int32_t id() const {
		return nanopbStruct.id;
	}

	inline int32_t type() const {
		return nanopbStruct.type;
	}

	inline int32_t result() const {
		return nanopbStruct.result;
	}

	inline std::string asr() {
		if (_asr.get() == NULL)
			return _empty_string;
		return *_asr;
	}

	inline std::string nlp() {
		if (_nlp.get() == NULL)
			return _empty_string;
		return *_nlp;
	}

	inline std::string action() {
		if (_action.get() == NULL)
			return _empty_string;
		return *_action;
	}

	inline std::string extra() {
		if (_extra.get() == NULL)
			return _empty_string;
		return *_extra;
	}

protected:
	void clear_super_data();

private:
	rokid_open_speech_v2_SpeechResponse nanopbStruct;
	std::shared_ptr<std::string> _asr;
	std::shared_ptr<std::string> _nlp;
	std::shared_ptr<std::string> _action;
	std::shared_ptr<std::string> _extra;
	std::string _empty_string;
};

} // namespace speech
} // namespace rokid
