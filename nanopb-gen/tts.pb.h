/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9 at Thu Jan 18 12:56:44 2018. */

#ifndef PB_ROKID_OPEN_SPEECH_V1_TTS_PB_H_INCLUDED
#define PB_ROKID_OPEN_SPEECH_V1_TTS_PB_H_INCLUDED
#include <pb.h>

#include "speech_types.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Struct definitions */
typedef struct _rokid_open_speech_v1_TtsRequest {
    int32_t id;
    pb_callback_t text;
    pb_callback_t declaimer;
    pb_callback_t codec;
    bool has_sample_rate;
    uint32_t sample_rate;
/* @@protoc_insertion_point(struct:rokid_open_speech_v1_TtsRequest) */
} rokid_open_speech_v1_TtsRequest;

typedef struct _rokid_open_speech_v1_TtsResponse {
    int32_t id;
    rokid_open_speech_v1_SpeechErrorCode result;
    pb_callback_t text;
    pb_callback_t voice;
    bool has_finish;
    bool finish;
/* @@protoc_insertion_point(struct:rokid_open_speech_v1_TtsResponse) */
} rokid_open_speech_v1_TtsResponse;

/* Default values for struct fields */

/* Initializer values for message structs */
#define rokid_open_speech_v1_TtsRequest_init_default {0, {{NULL}, NULL}, {{NULL}, NULL}, {{NULL}, NULL}, false, 0}
#define rokid_open_speech_v1_TtsResponse_init_default {0, (rokid_open_speech_v1_SpeechErrorCode)0, {{NULL}, NULL}, {{NULL}, NULL}, false, 0}
#define rokid_open_speech_v1_TtsRequest_init_zero {0, {{NULL}, NULL}, {{NULL}, NULL}, {{NULL}, NULL}, false, 0}
#define rokid_open_speech_v1_TtsResponse_init_zero {0, (rokid_open_speech_v1_SpeechErrorCode)0, {{NULL}, NULL}, {{NULL}, NULL}, false, 0}

/* Field tags (for use in manual encoding/decoding) */
#define rokid_open_speech_v1_TtsRequest_id_tag   1
#define rokid_open_speech_v1_TtsRequest_text_tag 2
#define rokid_open_speech_v1_TtsRequest_declaimer_tag 3
#define rokid_open_speech_v1_TtsRequest_codec_tag 4
#define rokid_open_speech_v1_TtsRequest_sample_rate_tag 5
#define rokid_open_speech_v1_TtsResponse_id_tag  1
#define rokid_open_speech_v1_TtsResponse_result_tag 2
#define rokid_open_speech_v1_TtsResponse_text_tag 3
#define rokid_open_speech_v1_TtsResponse_voice_tag 4
#define rokid_open_speech_v1_TtsResponse_finish_tag 5

/* Struct field encoding specification for nanopb */
extern const pb_field_t rokid_open_speech_v1_TtsRequest_fields[6];
extern const pb_field_t rokid_open_speech_v1_TtsResponse_fields[6];

/* Maximum encoded size of messages (where known) */
/* rokid_open_speech_v1_TtsRequest_size depends on runtime parameters */
/* rokid_open_speech_v1_TtsResponse_size depends on runtime parameters */

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define TTS_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
