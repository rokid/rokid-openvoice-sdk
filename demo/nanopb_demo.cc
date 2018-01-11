#include <stdio.h>
#include "nanopb_encoder.h"

using namespace rokid::speech;
using std::string;

/**
static bool encode_string(pb_ostream_t* stream, const pb_field_t* field, void* const* arg) {
	if (!pb_encode_tag_for_field(stream, field))
		return false;
	if (arg == NULL)
		return false;
	pb_byte_t* str = (pb_byte_t*)(*arg);
	return pb_encode_string(stream, str, strlen((char*)str));
}

static bool decode_string(pb_istream_t *stream, const pb_field_t *field, void **arg) {
	pb_byte_t* buf;
	printf("decode string: bytes left %lu\n", stream->bytes_left);
	buf = new pb_byte_t[stream->bytes_left];
	if (!pb_read(stream, buf, stream->bytes_left)) {
		delete buf;
		return false;
	}
	*arg = buf;
	return true;
}
*/

static void test_auth() {
	AuthRequest req;
	std::string buf;

	req.set_key("rokid_test_key");
	req.set_device_type_id("rokid_test_device_type_id");
	req.set_device_id("rokid_test_device_id");
	if (!req.SerializeToString(&buf)) {
		printf("pb serialize failed\n");
		return;
	}
	printf("pb serialize bytes %lu\n", buf.length());
	/**
	AuthRequest req = AuthRequest_init_default;
	pb_byte_t buf[256];
	pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));

	req.key.arg = (void*)"rokid_test_key";
	req.key.funcs.encode = encode_string;
	req.device_type_id.arg = (void*)"rokid_test_device_type_id";
	req.device_type_id.funcs.encode = encode_string;
	req.device_id.arg = (void*)"rokid_test_device_id";
	req.device_id.funcs.encode = encode_string;
	req.service.arg = (void*)"tts";
	req.service.funcs.encode = encode_string;
	req.version.arg = (void*)"1";
	req.version.funcs.encode = encode_string;
	req.timestamp.arg = (void*)"0123456789abcdef";
	req.timestamp.funcs.encode = encode_string;
	req.sign.arg = (void*)"fedcba9876543210";
	req.sign.funcs.encode = encode_string;
	if (!pb_encode(&stream, AuthRequest_fields, &req)) {
		printf("pb encode failed\n");
		return;
	}
	printf("pb encode bytes %lu\n", stream.bytes_written);

	pb_istream_t istream = pb_istream_from_buffer(buf, stream.bytes_written);
	req.key.funcs.decode = decode_string;
	req.device_type_id.funcs.decode = decode_string;
	req.device_id.funcs.decode = decode_string;
	req.service.funcs.decode = decode_string;
	req.version.funcs.decode = decode_string;
	req.timestamp.funcs.decode = decode_string;
	req.sign.funcs.decode = decode_string;
	if (!pb_decode(&istream, AuthRequest_fields, &req)) {
		printf("pb decode failed\n");
		return;
	}

	printf("key = %s, device type = %s, device id = %s\n",
			req.key.arg, req.device_type_id.arg, req.device_id.arg);
	printf("service = %s, version = %s, timestamp = %s, sign = %s\n",
			req.service.arg, req.version.arg, req.timestamp.arg, req.sign.arg);
	delete req.key.arg;
	delete req.device_type_id.arg;
	delete req.device_id.arg;
	delete req.service.arg;
	delete req.version.arg;
	delete req.timestamp.arg;
	delete req.sign.arg;
	*/
}

static void test_tts() {
}

static void test_speech() {
	/**
	SpeechRequest req = SpeechRequest_init_default;
	pb_byte_t buf[256];
	pb_ostream_t ostream = pb_ostream_from_buffer(buf, sizeof(buf));

	req.id = 1;
	req.type = ReqType_START;
	req.asr.funcs.encode = encode_string;
	req.asr.arg = (void*)"hello";
	req.has_options = false;
	if (!pb_encode(&ostream, SpeechRequest_fields, &req)) {
		printf("pb encode failed\n");
		return;
	}
	printf("pb encode bytes %lu\n", ostream.bytes_written);

	pb_istream_t istream = pb_istream_from_buffer(buf, ostream.bytes_written);
	req.voice.funcs.decode = decode_string;
	req.asr.funcs.decode = decode_string;
	if (!pb_decode(&istream, SpeechRequest_fields, &req)) {
		printf("pb decode failed\n");
		return;
	}

	printf("id = %d, type = %d\n", req.id, req.type);
	printf("voice = %s\n", req.voice.arg ? (char*)req.voice.arg : "null");
	printf("asr = %s\n", req.asr.arg ? (char*)req.asr.arg : "null");
	*/
}

int main(int argc, char** argv) {
	test_auth();
	test_tts();
	test_speech();
	return 0;
}
