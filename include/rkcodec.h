#pragma once

#include <stdint.h>
#include "opus.h"

namespace rokid {
namespace speech {

// opus -->  pcm
class RKOpusDecoder {
public:
	RKOpusDecoder();

	~RKOpusDecoder();

	// params:
	//   sample_rate: pcm sample rate
	//   duration: duration(ms) per opus frame
	//   bitrate: opus bitrate
	bool init(uint32_t sample_rate, uint32_t bitrate, uint32_t duration);

	inline uint32_t opu_frame_size() const {
		return _opu_frame_size;
	}

	uint32_t pcm_frame_size() const {
		return _pcm_frame_size;
	}

	// param 'opu': opu frame data, length must be 'opu_frame_size'
	// return pcm frame data, length is 'pcm_frame_size' * sizeof(uint16_t)
	const uint16_t* decode_frame(const void* opu);

	void close();

private:
	// bytes per opu frame
	uint32_t _opu_frame_size;
	// samples per pcm frame
	uint32_t _pcm_frame_size;
	OpusDecoder* _opus_decoder;
	uint16_t* _pcm_buffer;
};

// pcm -->  opus
class RKOpusEncoder {
public:
	RKOpusEncoder();

	~RKOpusEncoder();

	// params:
	//   sample_rate: pcm sample rate
	//   duration: duration(ms) per opus frame
	//   bitrate: opus bitrate
	bool init(uint32_t sample_rate, uint32_t bitrate, uint32_t duration);

	uint32_t pcm_frame_size() const {
		return _pcm_frame_size;
	}

	// param 'pcm': pcm frame data, length must be 'num_frames' * pcm_frame_size
	//       'enc_frames':  encoded pcm frames in this invocation of 'encode'
	//       'enc_size':  opus data size in bytes in this invocation of 'encode'
	// return opus data, length is 'enc_size'
	const uint8_t* encode(const uint16_t* pcm, uint32_t num_frames,
			uint32_t& enc_frames, uint32_t& enc_size);

	void close();

private:
	// samples per pcm frame
	uint32_t _pcm_frame_size;
	OpusEncoder* _opus_encoder;
	uint8_t* _opus_buffer;
};

} // namespace speech
} // namesapce rokid
