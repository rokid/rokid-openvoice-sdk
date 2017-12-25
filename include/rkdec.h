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

} // namespace speech
} // namesapce rokid
