#include <sys/mman.h>
#include "nanopb_encoder.h"
#include "pb_encode.h"

using std::string;

namespace rokid {
namespace speech {

Buffer::Buffer(uint32_t size) {
	data = mmap(NULL, size, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE, -1 , 0);
	dataSize = size;
}

Buffer::~Buffer() {
	munmap(data, dataSize);
}

void* Buffer::lock() {
	_mutex.lock();
	return data;
}

void Buffer::unlock() {
	_mutex.unlock();
}

Buffer NanoPBEncoder::encodeBuffer(1024 * 1024);

bool NanoPBEncoder::SerializeToString(string* str) {
	pb_byte_t* buf = (pb_byte_t*)encodeBuffer.lock();
	pb_ostream_t ostream = pb_ostream_from_buffer(buf, encodeBuffer.size());
	bool r = pb_encode(&ostream, nanopbFields, nanopbStructPointer);
	if (r)
		str->assign((char*)buf, ostream.bytes_written);
	encodeBuffer.unlock();
	return r;
}

bool NanoPBEncoder::encodeAsSubMsg(pb_ostream_t* stream) {
	return pb_encode_submessage(stream, nanopbFields, nanopbStructPointer);
}

void NanoPBEncoder::set_string_field(pb_callback_t* cbt, const string& str) {
	cbt->funcs.encode = encode_string;
	cbt->arg = (void*)&str;
}

bool NanoPBEncoder::encode_string(pb_ostream_t* stream,
		const pb_field_t* field, void* const* arg) {
	if (!pb_encode_tag_for_field(stream, field))
		return false;
	if (arg == NULL)
		return false;
	string* str = (string*)(*arg);
	return pb_encode_string(stream, (const pb_byte_t*)str->data(), str->length());
}

bool NanoPBEncoder::encode_submsg(pb_ostream_t* stream,
		const pb_field_t* field, void* const* arg) {
	if (!pb_encode_tag_for_field(stream, field))
		return false;
	if (arg == NULL)
		return false;
	NanoPBEncoder* encoder = (NanoPBEncoder*)arg;
	return encoder->encodeAsSubMsg(stream);
}

AuthRequest::AuthRequest() {
	nanopbStruct = rokid_open_speech_AuthRequest_init_default;
	nanopbStructPointer = &nanopbStruct;
	nanopbFields = rokid_open_speech_AuthRequest_fields;
}

AuthRequest::~AuthRequest() {
}

void AuthRequest::set_key(const string& str) {
	_key = str;
	set_string_field(&nanopbStruct.key, _key);
}

void AuthRequest::set_device_type_id(const string& str) {
	_device_type_id = str;
	set_string_field(&nanopbStruct.device_type_id, _device_type_id);
}

void AuthRequest::set_device_id(const string& str) {
	_device_id = str;
	set_string_field(&nanopbStruct.device_id, _device_id);
}

void AuthRequest::set_service(const string& str) {
	_service = str;
	set_string_field(&nanopbStruct.service, _service);
}

void AuthRequest::set_version(const string& str) {
	_version = str;
	set_string_field(&nanopbStruct.version, _version);
}

void AuthRequest::set_timestamp(const string& str) {
	_timestamp = str;
	set_string_field(&nanopbStruct.timestamp, _timestamp);
}

void AuthRequest::set_sign(const string& str) {
	_sign = str;
	set_string_field(&nanopbStruct.sign, _sign);
}

TtsRequest::TtsRequest() {
	nanopbStruct = rokid_open_speech_v1_TtsRequest_init_default;
	nanopbStructPointer = &nanopbStruct;
	nanopbFields = rokid_open_speech_v1_TtsRequest_fields;
}

void TtsRequest::set_id(int32_t i) {
	nanopbStruct.id = i;
}

void TtsRequest::set_text(const string& str) {
	_text = str;
	set_string_field(&nanopbStruct.text, _text);
}

void TtsRequest::set_declaimer(const string& str) {
	_declaimer = str;
	set_string_field(&nanopbStruct.declaimer, _declaimer);
}

void TtsRequest::set_codec(const string& str) {
	_codec = str;
	set_string_field(&nanopbStruct.codec, _codec);
}

void TtsRequest::set_sample_rate(uint32_t i) {
	nanopbStruct.has_sample_rate = true;
	nanopbStruct.sample_rate = i;
}

SpeechOptionsEnc::SpeechOptionsEnc(void* st) {
	nanopbStruct = (rokid_open_speech_v2_SpeechOptions*)st;
	nanopbStructPointer = st;
	nanopbFields = rokid_open_speech_v2_SpeechOptions_fields;
}

void SpeechOptionsEnc::set_lang(int32_t i) {
	nanopbStruct->lang = (rokid_open_speech_v2_Lang)i;
}

void SpeechOptionsEnc::set_codec(int32_t i) {
	nanopbStruct->codec = (rokid_open_speech_v1_Codec)i;
}

void SpeechOptionsEnc::set_vad_mode(int32_t i) {
	nanopbStruct->vad_mode = (rokid_open_speech_v2_VadMode)i;
}

void SpeechOptionsEnc::set_vend_timeout(uint32_t i) {
	nanopbStruct->has_vend_timeout = true;
	nanopbStruct->vend_timeout = i;
}

void SpeechOptionsEnc::set_no_nlp(bool b) {
	nanopbStruct->no_nlp = b;
}

void SpeechOptionsEnc::set_no_intermediate_asr(bool b) {
	nanopbStruct->no_intermediate_asr = b;
}

void SpeechOptionsEnc::set_stack(const string& str) {
	_stack = str;
	set_string_field(&nanopbStruct->stack, _stack);
}

void SpeechOptionsEnc::set_voice_trigger(const string& str) {
	_voice_trigger = str;
	set_string_field(&nanopbStruct->voice_trigger, _voice_trigger);
}

void SpeechOptionsEnc::set_voice_power(float f) {
	nanopbStruct->has_voice_power = true;
	nanopbStruct->voice_power = f;
}

void SpeechOptionsEnc::set_trigger_start(uint32_t i) {
	nanopbStruct->has_trigger_start = true;
	nanopbStruct->trigger_start = i;
}

void SpeechOptionsEnc::set_trigger_length(uint32_t i) {
	nanopbStruct->has_trigger_length = true;
	nanopbStruct->trigger_length = i;
}

void SpeechOptionsEnc::set_skill_options(const string& str) {
	_skill_options = str;
	set_string_field(&nanopbStruct->skill_options, _skill_options);
}

void SpeechOptionsEnc::set_voice_extra(const string& str) {
	_voice_extra = str;
	set_string_field(&nanopbStruct->voice_extra, _voice_extra);
}

SpeechRequest::SpeechRequest() {
	nanopbStruct = rokid_open_speech_v2_SpeechRequest_init_default;
	nanopbStructPointer = &nanopbStruct;
	nanopbFields = rokid_open_speech_v2_SpeechRequest_fields;
	_options = NULL;
}

SpeechRequest::~SpeechRequest() {
	if (_options)
		delete _options;
}

void SpeechRequest::set_id(int32_t i) {
	nanopbStruct.id = i;
}

void SpeechRequest::set_type(int32_t i) {
	nanopbStruct.type = (rokid_open_speech_v1_ReqType)i;
}

void SpeechRequest::set_voice(const string& str) {
	_voice = str;
	set_string_field(&nanopbStruct.voice, _voice);
}

void SpeechRequest::set_asr(const string& str) {
	_asr = str;
	set_string_field(&nanopbStruct.asr, _asr);
}

SpeechOptionsEnc* SpeechRequest::mutable_options() {
	_options = new SpeechOptionsEnc(&nanopbStruct.options);
	nanopbStruct.has_options = true;
	return _options;
}

PingPayload::PingPayload() {
	nanopbStruct = rokid_open_speech_v1_PingPayload_init_default;
	nanopbStructPointer = &nanopbStruct;
	nanopbFields = rokid_open_speech_v1_PingPayload_fields;
}

void PingPayload::set_req_id(int32_t i) {
	nanopbStruct.req_id = i;
}

void PingPayload::set_now_tp(uint64_t i) {
	nanopbStruct.now_tp = i;
}

void PingPayload::set_req_tp(uint64_t i) {
	nanopbStruct.req_tp = i;
}

void PingPayload::set_resp_tp(uint64_t i) {
	nanopbStruct.req_tp = i;
}

} // namespace speech
} // namespace rokid
