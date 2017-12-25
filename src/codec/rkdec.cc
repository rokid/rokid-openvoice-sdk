#include <stdio.h>
#include "rkdec.h"
#include "log.h"

#define TAG "rokid.opus.decoder"

namespace rokid {
namespace speech {

RKOpusDecoder::RKOpusDecoder() : _opu_frame_size(0),
		_pcm_frame_size(0), _opus_decoder(NULL), _pcm_buffer(NULL) {
}

RKOpusDecoder::~RKOpusDecoder() {
	close();
}

bool RKOpusDecoder::init(uint32_t sample_rate, uint32_t bitrate,
		uint32_t duration) {
	if (_opus_decoder) {
		Log::w(TAG, "init failed: already initialized");
		return false;
	}
	int err;
	_opus_decoder = opus_decoder_create(sample_rate, 1, &err);
	if (err != OPUS_OK) {
		Log::w(TAG, "opus decoder create failed: %d\n", err);
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
		Log::w(TAG, "decode failed: not init");
		return NULL;
	}
	if (opu == NULL) {
		Log::w(TAG, "decode failed: opu data is null");
		return NULL;
	}
	int c = opus_decode(_opus_decoder, reinterpret_cast<const uint8_t*>(opu),
			_opu_frame_size, reinterpret_cast<opus_int16*>(_pcm_buffer),
			_pcm_frame_size, 0);
	if (c < 0) {
		Log::w(TAG, "decode failed: opu_decode error %d", c);
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

} // namespace speech
} // namespace rokid
