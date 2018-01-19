#include "nanopb_decoder.h"
#include "pb_decode.h"

using std::string;
using std::shared_ptr;
using std::make_shared;

namespace rokid {
namespace speech {

bool NanoPBDecoder::ParseFromArray(const char* data, uint32_t length) {
	clear_super_data();
	pb_istream_t istream = pb_istream_from_buffer((const pb_byte_t*)data, length);
	return pb_decode(&istream, nanopbFields, nanopbStructPointer);
}

bool NanoPBDecoder::decode_string(pb_istream_t *stream,
		const pb_field_t *field, void **arg) {
	pb_byte_t* buf;
	size_t bytes_count = stream->bytes_left;
	if (bytes_count == 0)
		return true;
	// TODO: 减少内存碎片
	buf = new pb_byte_t[bytes_count];
	if (!pb_read(stream, buf, bytes_count)) {
		delete buf;
		return false;
	}
	if (*arg == NULL) {
		delete buf;
		return false;
	}
	shared_ptr<string>* spp = (shared_ptr<string>*)(*arg);
	*spp = make_shared<string>((char*)buf, bytes_count);
	delete buf;
	return true;
}

void NanoPBDecoder::init_string_field(pb_callback_t* cb,
		shared_ptr<string>* strp) {
	cb->funcs.decode = decode_string;
	cb->arg = strp;
}

AuthResponse::AuthResponse() {
	nanopbStruct = rokid_open_speech_AuthResponse_init_default;
	nanopbStructPointer = &nanopbStruct;
	nanopbFields = rokid_open_speech_AuthResponse_fields;
}

TtsResponse::TtsResponse() {
	nanopbStruct = rokid_open_speech_v1_TtsResponse_init_default;
	nanopbStructPointer = &nanopbStruct;
	nanopbFields = rokid_open_speech_v1_TtsResponse_fields;
	init_string_field(&nanopbStruct.text, &_text);
	init_string_field(&nanopbStruct.voice, &_voice);
}

string* TtsResponse::release_voice() {
	if (_voice.get() == NULL)
		return NULL;
	string* str = new string(*_voice);
	_voice.reset();
	return str;
}

void TtsResponse::clear_super_data() {
	_text.reset();
	_voice.reset();
}

SpeechResponse::SpeechResponse() {
	nanopbStruct = rokid_open_speech_v2_SpeechResponse_init_default;
	nanopbStructPointer = &nanopbStruct;
	nanopbFields = rokid_open_speech_v2_SpeechResponse_fields;
	init_string_field(&nanopbStruct.asr, &_asr);
	init_string_field(&nanopbStruct.nlp, &_nlp);
	init_string_field(&nanopbStruct.action, &_action);
	init_string_field(&nanopbStruct.extra, &_extra);
}

void SpeechResponse::clear_super_data() {
	_asr.reset();
	_nlp.reset();
	_action.reset();
	_extra.reset();
}

} // namespace speech
} // namespace rokid
