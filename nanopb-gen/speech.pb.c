/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9.3 at Tue Apr  2 19:57:31 2019. */

#include "speech.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



const pb_field_t rokid_open_speech_v2_SpeechOptions_fields[16] = {
    PB_FIELD(  1, UENUM   , OPTIONAL, STATIC  , FIRST, rokid_open_speech_v2_SpeechOptions, lang, lang, 0),
    PB_FIELD(  2, UENUM   , OPTIONAL, STATIC  , OTHER, rokid_open_speech_v2_SpeechOptions, codec, lang, 0),
    PB_FIELD(  3, UENUM   , OPTIONAL, STATIC  , OTHER, rokid_open_speech_v2_SpeechOptions, vad_mode, codec, 0),
    PB_FIELD(  4, UINT32  , OPTIONAL, STATIC  , OTHER, rokid_open_speech_v2_SpeechOptions, vend_timeout, vad_mode, 0),
    PB_FIELD(  5, BOOL    , OPTIONAL, STATIC  , OTHER, rokid_open_speech_v2_SpeechOptions, no_nlp, vend_timeout, 0),
    PB_FIELD(  6, BOOL    , OPTIONAL, STATIC  , OTHER, rokid_open_speech_v2_SpeechOptions, no_intermediate_asr, no_nlp, 0),
    PB_FIELD(  7, STRING  , OPTIONAL, CALLBACK, OTHER, rokid_open_speech_v2_SpeechOptions, stack, no_intermediate_asr, 0),
    PB_FIELD(  8, STRING  , OPTIONAL, CALLBACK, OTHER, rokid_open_speech_v2_SpeechOptions, voice_trigger, stack, 0),
    PB_FIELD(  9, FLOAT   , OPTIONAL, STATIC  , OTHER, rokid_open_speech_v2_SpeechOptions, voice_power, voice_trigger, 0),
    PB_FIELD( 10, UINT32  , OPTIONAL, STATIC  , OTHER, rokid_open_speech_v2_SpeechOptions, trigger_start, voice_power, 0),
    PB_FIELD( 11, UINT32  , OPTIONAL, STATIC  , OTHER, rokid_open_speech_v2_SpeechOptions, trigger_length, trigger_start, 0),
    PB_FIELD( 12, STRING  , OPTIONAL, CALLBACK, OTHER, rokid_open_speech_v2_SpeechOptions, skill_options, trigger_length, 0),
    PB_FIELD( 13, STRING  , OPTIONAL, CALLBACK, OTHER, rokid_open_speech_v2_SpeechOptions, voice_extra, skill_options, 0),
    PB_FIELD( 14, UINT32  , OPTIONAL, STATIC  , OTHER, rokid_open_speech_v2_SpeechOptions, vad_begin, voice_extra, 0),
    PB_FIELD( 15, BOOL    , OPTIONAL, STATIC  , OTHER, rokid_open_speech_v2_SpeechOptions, no_trigger_confirm, vad_begin, 0),
    PB_LAST_FIELD
};

const pb_field_t rokid_open_speech_v2_SpeechRequest_fields[6] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, rokid_open_speech_v2_SpeechRequest, id, id, 0),
    PB_FIELD(  2, UENUM   , REQUIRED, STATIC  , OTHER, rokid_open_speech_v2_SpeechRequest, type, id, 0),
    PB_FIELD(  3, BYTES   , OPTIONAL, CALLBACK, OTHER, rokid_open_speech_v2_SpeechRequest, voice, type, 0),
    PB_FIELD(  4, STRING  , OPTIONAL, CALLBACK, OTHER, rokid_open_speech_v2_SpeechRequest, asr, voice, 0),
    PB_FIELD(  5, MESSAGE , OPTIONAL, STATIC  , OTHER, rokid_open_speech_v2_SpeechRequest, options, asr, &rokid_open_speech_v2_SpeechOptions_fields),
    PB_LAST_FIELD
};

const pb_field_t rokid_open_speech_v2_SpeechResponse_fields[12] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, rokid_open_speech_v2_SpeechResponse, id, id, 0),
    PB_FIELD(  2, UENUM   , REQUIRED, STATIC  , OTHER, rokid_open_speech_v2_SpeechResponse, type, id, 0),
    PB_FIELD(  3, UENUM   , REQUIRED, STATIC  , OTHER, rokid_open_speech_v2_SpeechResponse, result, type, 0),
    PB_FIELD(  4, STRING  , OPTIONAL, CALLBACK, OTHER, rokid_open_speech_v2_SpeechResponse, asr, result, 0),
    PB_FIELD(  5, STRING  , OPTIONAL, CALLBACK, OTHER, rokid_open_speech_v2_SpeechResponse, nlp, asr, 0),
    PB_FIELD(  6, STRING  , OPTIONAL, CALLBACK, OTHER, rokid_open_speech_v2_SpeechResponse, action, nlp, 0),
    PB_FIELD(  7, STRING  , OPTIONAL, CALLBACK, OTHER, rokid_open_speech_v2_SpeechResponse, extra, action, 0),
    PB_FIELD(  8, FLOAT   , REPEATED, CALLBACK, OTHER, rokid_open_speech_v2_SpeechResponse, asr_scores, extra, 0),
    PB_FIELD(  9, STRING  , OPTIONAL, CALLBACK, OTHER, rokid_open_speech_v2_SpeechResponse, vpr, asr_scores, 0),
    PB_FIELD( 10, STRING  , OPTIONAL, CALLBACK, OTHER, rokid_open_speech_v2_SpeechResponse, asr_phoneticisms, vpr, 0),
    PB_FIELD( 11, STRING  , OPTIONAL, CALLBACK, OTHER, rokid_open_speech_v2_SpeechResponse, voice_trigger, asr_phoneticisms, 0),
    PB_LAST_FIELD
};





/* Check that field information fits in pb_field_t */
#if !defined(PB_FIELD_32BIT)
/* If you get an error here, it means that you need to define PB_FIELD_32BIT
 * compile-time option. You can do that in pb.h or on compiler command line.
 * 
 * The reason you need to do this is that some of your messages contain tag
 * numbers or field sizes that are larger than what can fit in 8 or 16 bit
 * field descriptors.
 */
PB_STATIC_ASSERT((pb_membersize(rokid_open_speech_v2_SpeechRequest, options) < 65536), YOU_MUST_DEFINE_PB_FIELD_32BIT_FOR_MESSAGES_rokid_open_speech_v2_SpeechOptions_rokid_open_speech_v2_SpeechRequest_rokid_open_speech_v2_SpeechResponse)
#endif

#if !defined(PB_FIELD_16BIT) && !defined(PB_FIELD_32BIT)
/* If you get an error here, it means that you need to define PB_FIELD_16BIT
 * compile-time option. You can do that in pb.h or on compiler command line.
 * 
 * The reason you need to do this is that some of your messages contain tag
 * numbers or field sizes that are larger than what can fit in the default
 * 8 bit descriptors.
 */
PB_STATIC_ASSERT((pb_membersize(rokid_open_speech_v2_SpeechRequest, options) < 256), YOU_MUST_DEFINE_PB_FIELD_16BIT_FOR_MESSAGES_rokid_open_speech_v2_SpeechOptions_rokid_open_speech_v2_SpeechRequest_rokid_open_speech_v2_SpeechResponse)
#endif


/* @@protoc_insertion_point(eof) */
