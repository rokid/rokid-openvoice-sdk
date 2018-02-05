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
	src/common/log.cc \
	src/common/log.h \
	src/common/speech_connection.cc \
	src/common/speech_connection.h \
	src/common/nanopb_encoder.cc \
	src/common/nanopb_encoder.h \
	src/common/nanopb_decoder.cc \
	src/common/nanopb_decoder.h

TTS_SRC := \
	src/tts/tts_impl.cc \
	src/tts/tts_impl.h \
	src/tts/types.h

SPEECH_SRC := \
	src/speech/speech_impl.cc \
	src/speech/speech_impl.h \
	src/speech/types.h

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
LOCAL_SHARED_LIBRARIES := liblog libuWS libcrypto libz
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

ifeq ($(PLATFORM_SDK_VERSION), 22)
LOCAL_SHARED_LIBRARIES += libc++ libdl
LOCAL_C_INCLUDES += external/libcxx/include
else ifeq ($(PLATFORM_SDK_VERSION), 19)
LOCAL_SDK_VERSION := 14
LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_CPPFLAGS := -D__STDC_FORMAT_MACROS
else
LOCAL_CXX_STL := libc++
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := librkcodec
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
	src/codec/rkcodec.cc \
	include/rkcodec.h \
	src/common/log.cc \
	src/common/log.h
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	external/libopus/include
LOCAL_SHARED_LIBRARIES := libopus
LOCAL_CPPFLAGS := -std=c++11
ifeq ($(PLATFORM_SDK_VERSION), 22)
LOCAL_SHARED_LIBRARIES += libc++ libdl
else ifeq ($(PLATFORM_SDK_VERSION), 19)
LOCAL_SDK_VERSION := 14
LOCAL_NDK_STL_VARIANT := gnustl_static
else
LOCAL_CXX_STL := libc++
endif
include $(BUILD_SHARED_LIBRARY)

# module speech_demo
include $(CLEAR_VARS)
LOCAL_MODULE := speech_demo
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
	demo/demo.cc \
	demo/simple_wave.cc \
	demo/speech_stress_test.cc
LOCAL_C_INCLUDES := \
	$(PROTOC_OUT_DIR) \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	external/boringssl/include
LOCAL_SHARED_LIBRARIES := libspeech
LOCAL_CPPFLAGS := $(COMMON_CFLAGS) \
	-std=c++11
ifeq ($(PLATFORM_SDK_VERSION), 22)
LOCAL_SHARED_LIBRARIES += libc++ libdl
else ifeq ($(PLATFORM_SDK_VERSION), 19)
LOCAL_SDK_VERSION := 14
LOCAL_NDK_STL_VARIANT := gnustl_static
else
LOCAL_CXX_STL := libc++
endif
include $(BUILD_EXECUTABLE)
