#include <stdio.h>
#include <sys/mman.h>
#include "rkcodec.h"
#include "log.h"

#define DEC_TAG "rokid.opus.decoder"
#define ENC_TAG "rokid.opus.encoder"

namespace rokid {
namespace speech {

static const uint32_t OPUS_BUFFER_SIZE = 16 * 1024;

RKOpusDecoder::RKOpusDecoder() : _opu_frame_size(0),
		_pcm_frame_size(0), _opus_decoder(NULL), _pcm_buffer(NULL) {
}

RKOpusDecoder::~RKOpusDecoder() {
	close();
}

bool RKOpusDecoder::init(uint32_t sample_rate, uint32_t bitrate,
		uint32_t duration) {
	if (_opus_decoder) {
		Log::w(DEC_TAG, "init failed: already initialized");
		return false;
	}
	int err;
	_opus_decoder = opus_decoder_create(sample_rate, 1, &err);
	if (err != OPUS_OK) {
		Log::w(DEC_TAG, "opus decoder create failed: %d\n", err);
		return false;
	}
	opus_decoder_ctl(_opus_decoder, OPUS_SET_VBR(0));
	opus_decoder_ctl(_opus_decoder, OPUS_SET_BITRATE(bitrate));

	_opu_frame_size = bitrate * duration / 8000;
	_pcm_frame_size = sample_rate * duration / 1000;
	_pcm_buffer = new uint16_t[_pcm_frame_size];
	return true;
}

const uint16_t* RKOpusDecoder::decode_frame(const void* opu) {
	if (_opus_decoder == NULL) {
		Log::w(DEC_TAG, "decode failed: not init");
		return NULL;
	}
	if (opu == NULL) {
		Log::w(DEC_TAG, "decode failed: opu data is null");
		return NULL;
	}
	int c = opus_decode(_opus_decoder, reinterpret_cast<const uint8_t*>(opu),
			_opu_frame_size, reinterpret_cast<opus_int16*>(_pcm_buffer),
			_pcm_frame_size, 0);
	if (c < 0) {
		Log::w(DEC_TAG, "decode failed: opu_decode error %d", c);
		return NULL;
	}
	// debug
	// printf("decode debug: %x %x %x %x\n", _pcm_buffer[240], _pcm_buffer[241], _pcm_buffer[242], _pcm_buffer[243]);
	return _pcm_buffer;
}

void RKOpusDecoder::close() {
	if (_opus_decoder) {
		delete _pcm_buffer;
		_opu_frame_size = 0;
		_pcm_frame_size = 0;
		opus_decoder_destroy(_opus_decoder);
		_opus_decoder = NULL;
	}
}


RKOpusEncoder::RKOpusEncoder() : _pcm_frame_size(0),
	_opus_encoder(NULL), _opus_buffer(NULL) {
}

RKOpusEncoder::~RKOpusEncoder() {
	close();
}

bool RKOpusEncoder::init(uint32_t sample_rate, uint32_t bitrate,
		uint32_t duration) {
	if (_opus_encoder) {
		Log::w(ENC_TAG, "init failed: already initialized");
		return false;
	}
	int err;
	_opus_encoder = opus_encoder_create(sample_rate, 1, OPUS_APPLICATION_VOIP, &err);
	if (err != OPUS_OK) {
		Log::w(ENC_TAG, "opus encoder create failed: %d\n", err);
		return false;
	}
	opus_encoder_ctl(_opus_encoder, OPUS_SET_VBR(1));
	opus_encoder_ctl(_opus_encoder, OPUS_SET_BITRATE(bitrate));
	opus_encoder_ctl(_opus_encoder, OPUS_SET_COMPLEXITY(8));
	opus_encoder_ctl(_opus_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));

	_pcm_frame_size = sample_rate * duration / 1000;
	_opus_buffer = (uint8_t*)mmap(NULL, OPUS_BUFFER_SIZE,
			PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	return true;
}

const uint8_t* RKOpusEncoder::encode(const uint16_t* pcm,
		uint32_t num_frames, uint32_t& enc_frames, uint32_t& enc_size) {
	if (_opus_encoder == NULL) {
		Log::w(ENC_TAG, "encode failed: not init");
		return NULL;
	}
	if (pcm == NULL || num_frames == 0) {
		Log::w(ENC_TAG, "encode failed: pcm data is null");
		return NULL;
	}

	uint32_t i;
	int c;

	enc_frames = 0;
	enc_size = 0;
	for(i = 0; i < num_frames; ++i) {
		c = opus_encode(_opus_encoder,
				reinterpret_cast<const opus_int16*>(pcm) + i * _pcm_frame_size,
				_pcm_frame_size, _opus_buffer + enc_size + 1,
				OPUS_BUFFER_SIZE - enc_size - 1);
		if (c < 0) {
			Log::w(ENC_TAG, "encode failed: opu_encode error %d", c);
			return NULL;
		}
		// TODO: assert c < 256
		_opus_buffer[enc_size] = c;
		++enc_frames;
		enc_size += c + 1;
	}
	return _opus_buffer;
}

void RKOpusEncoder::close() {
	if (_opus_encoder) {
		munmap(_opus_buffer, OPUS_BUFFER_SIZE);
		_pcm_frame_size = 0;
		opus_encoder_destroy(_opus_encoder);
		_opus_encoder = NULL;
	}
}

} // namespace speech
} // namespace rokid
