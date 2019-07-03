include $(CLEAR_VARS)

LOCAL_MODULE := libspeech
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc

PROTOC_GEN_SRC := \
	nanopb-gen/speech_types.pb.c \
	nanopb-gen/auth.pb.c \
	nanopb-gen/tts.pb.c \
	nanopb-gen/speech.pb.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	$(LOCAL_PATH)/nanopb \
	$(LOCAL_PATH)/nanopb-gen \
	external/zlib

COMMON_SRC := \
	src/common/speech_connection.cc \
	src/common/nanopb_encoder.cc \
	src/common/nanopb_decoder.cc \
	src/common/alt_chrono.cc \
	src/common/log_plugin.cc \
	src/common/trace-uploader.cc

TTS_SRC := \
	src/tts/tts_impl.cc

SPEECH_SRC := \
	src/speech/speech_impl.cc

NANOPB_SRC := \
	nanopb/pb_common.c \
	nanopb/pb_decode.c \
	nanopb/pb_encode.c

LOCAL_SRC_FILES := \
	$(COMMON_SRC) \
	$(TTS_SRC) \
	$(SPEECH_SRC) \
	$(PROTOC_GEN_SRC) \
	$(NANOPB_SRC)

LOCAL_CPPFLAGS := $(COMMON_CFLAGS) -std=c++11
LOCAL_SHARED_LIBRARIES := librlog libuWS libcrypto libz
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := librkcodec
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
	src/codec/rkcodec.cc
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	external/libopus/include
LOCAL_SHARED_LIBRARIES := libopus libspeech librlog
LOCAL_CPPFLAGS := -std=c++11
include $(BUILD_SHARED_LIBRARY)
