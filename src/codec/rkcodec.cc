#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include "rkcodec.h"
#include "rlog.h"

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
		KLOGW(DEC_TAG, "already initialized");
		return true;
	}
	int err;
	_opus_decoder = opus_decoder_create(sample_rate, 1, &err);
	if (err != OPUS_OK) {
		KLOGW(DEC_TAG, "opus decoder create failed: %d\n", err);
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
		KLOGW(DEC_TAG, "decode failed: not init");
		return NULL;
	}
	if (opu == NULL) {
		KLOGW(DEC_TAG, "decode failed: opu data is null");
		return NULL;
	}
	int c = opus_decode(_opus_decoder, reinterpret_cast<const uint8_t*>(opu),
			_opu_frame_size, reinterpret_cast<opus_int16*>(_pcm_buffer),
			_pcm_frame_size, 0);
	if (c < 0) {
		KLOGW(DEC_TAG, "decode failed: opu_decode error %d", c);
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
		return true;
	}
	int err;
	_opus_encoder = opus_encoder_create(sample_rate, 1, OPUS_APPLICATION_VOIP, &err);
	if (err != OPUS_OK) {
		KLOGW(ENC_TAG, "opus encoder create failed: %d\n", err);
		return false;
	}
	opus_encoder_ctl(_opus_encoder, OPUS_SET_VBR(1));
	opus_encoder_ctl(_opus_encoder, OPUS_SET_BITRATE(bitrate));
	opus_encoder_ctl(_opus_encoder, OPUS_SET_COMPLEXITY(8));
	opus_encoder_ctl(_opus_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));

	_pcm_frame_size = sample_rate * duration / 1000;
	_opus_buffer = (uint8_t*)mmap(NULL, OPUS_BUFFER_SIZE,
			PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	a_pcm_frame = new uint16_t[_pcm_frame_size];
	pcm_frame_used_bytes = 0;
	return true;
}

const uint8_t* RKOpusEncoder::encode(const uint16_t* pcm,
		uint32_t size, uint32_t& enc_size) {
	enc_size = 0;
	if (_opus_encoder == NULL) {
		KLOGW(ENC_TAG, "encode failed: not init");
		return NULL;
	}
	if (pcm == NULL || size == 0) {
		KLOGW(ENC_TAG, "encode failed: pcm data is null");
		return NULL;
	}


	int c;
	// encode remain pcm data of previous invocation
	if (pcm_frame_used_bytes) {
		uint32_t need = _pcm_frame_size - pcm_frame_used_bytes;
		if (need > size) {
			memcpy(a_pcm_frame + pcm_frame_used_bytes, pcm, size * sizeof(uint16_t));
			pcm_frame_used_bytes += size;
			return NULL;
		}
		memcpy(a_pcm_frame + pcm_frame_used_bytes, pcm, need * sizeof(uint16_t));
		c = opus_encode(_opus_encoder,
				reinterpret_cast<opus_int16*>(a_pcm_frame),
				_pcm_frame_size, _opus_buffer + enc_size + 1,
				OPUS_BUFFER_SIZE - enc_size - 1);
		if (c < 0) {
			KLOGW(ENC_TAG, "encode failed1: opu_encode error %d", c);
			return NULL;
		}
		_opus_buffer[enc_size] = c;
		enc_size += c + 1;
		pcm_frame_used_bytes = 0;
		pcm += need;
		size -= need;
	}

	while (true) {
		if (size >= _pcm_frame_size) {
			c = opus_encode(_opus_encoder,
					reinterpret_cast<const opus_int16*>(pcm),
					_pcm_frame_size, _opus_buffer + enc_size + 1,
					OPUS_BUFFER_SIZE - enc_size - 1);
			if (c < 0) {
				KLOGW(ENC_TAG, "encode failed2: opu_encode error %d", c);
				return NULL;
			}
			// TODO: assert c < 256
			_opus_buffer[enc_size] = c;
			enc_size += c + 1;
			pcm += _pcm_frame_size;
			size -= _pcm_frame_size;
		} else {
			if (size) {
				memcpy(a_pcm_frame, pcm, size * sizeof(uint16_t));
				pcm_frame_used_bytes = size;
			}
			break;
		}
	}
	return _opus_buffer;
}

void RKOpusEncoder::close() {
	if (_opus_encoder) {
		delete a_pcm_frame;
		pcm_frame_used_bytes = 0;
		munmap(_opus_buffer, OPUS_BUFFER_SIZE);
		_pcm_frame_size = 0;
		opus_encoder_destroy(_opus_encoder);
		_opus_encoder = NULL;
	}
}

} // namespace speech
} // namespace rokid
