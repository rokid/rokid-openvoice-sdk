include $(CLEAR_VARS)

LOCAL_MODULE := libspeech
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc

# protobuf source generate
SPEECH_PROTO_FILE := \
	$(LOCAL_PATH)/proto/speech_types.proto \
	$(LOCAL_PATH)/proto/auth.proto \
	$(LOCAL_PATH)/proto/tts.proto \
	$(LOCAL_PATH)/proto/speech.proto
ifeq ($(PLATFORM_SDK_VERSION), 19)
PROTOC_OUT_DIR := $(call local-intermediates-dir)/gen
else
PROTOC_OUT_DIR := $(call local-generated-sources-dir)
endif
PROTOC_GEN_SRC := \
	speech_types.pb.cc \
	auth.pb.cc \
	tts.pb.cc \
	speech.pb.cc
PROTOC_GEN_SRC_L := $(addprefix $(PROTOC_OUT_DIR)/, $(PROTOC_GEN_SRC))
protoc_stamp := $(PROTOC_OUT_DIR)/stamp
$(protoc_stamp): PRIVATE_CUSTOM_TOOL := $(PROTOC) -I$(LOCAL_PATH)/proto --cpp_out=$(PROTOC_OUT_DIR) $(SPEECH_PROTO_FILE) && touch $(protoc_stamp)
$(protoc_stamp): $(SPEECH_PROTO_FILE) $(PROTOC)
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES := $(PROTOC_GEN_SRC_L)

LOCAL_C_INCLUDES := \
	$(PROTOC_OUT_DIR) \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	external/zlib

$(PROTOC_GEN_SRC_L): $(protoc_stamp)

COMMON_SRC := \
	src/common/log.cc \
	src/common/log.h \
	src/common/speech_connection.cc \
	src/common/speech_connection.h

TTS_SRC := \
	src/tts/tts_impl.cc \
	src/tts/tts_impl.h \
	src/tts/types.h

SPEECH_SRC := \
	src/speech/speech_impl.cc \
	src/speech/speech_impl.h \
	src/speech/types.h

LOCAL_SRC_FILES := \
	$(COMMON_SRC) \
	$(TTS_SRC) \
	$(SPEECH_SRC)

LOCAL_CFLAGS := $(COMMON_CFLAGS) \
	-std=c++11 -frtti -fexceptions
LOCAL_SHARED_LIBRARIES := liblog libuWS libcrypto libz
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

ifeq ($(PLATFORM_SDK_VERSION), 23)
LOCAL_SHARED_LIBRARIES += libprotobuf-rokid-cpp-full
LOCAL_CXX_STL := libc++
else ifeq ($(PLATFORM_SDK_VERSION), 22)
LOCAL_SHARED_LIBRARIES += libc++ libdl libprotobuf-rokid-cpp-full
LOCAL_C_INCLUDES += external/libcxx/include
LOCAL_CPPFLAGS := -DLOW_PB_VERSION
else
LOCAL_STATIC_LIBRARIES := libprotobuf-cpp-2.3.0-full-gnustl-rtti
LOCAL_SDK_VERSION := 14
LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_CPPFLAGS := -D__STDC_FORMAT_MACROS -DLOW_PB_VERSION
LOCAL_C_INCLUDES += external/protobuf/src
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
ifeq ($(PLATFORM_SDK_VERSION), 23)
LOCAL_CXX_STL := libc++
else ifeq ($(PLATFORM_SDK_VERSION), 22)
LOCAL_SHARED_LIBRARIES += libc++ libdl
else
LOCAL_SDK_VERSION := 14
LOCAL_NDK_STL_VARIANT := gnustl_static
endif
include $(BUILD_SHARED_LIBRARY)

# module speech_demo
include $(CLEAR_VARS)
LOCAL_MODULE := speech_demo
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
	demo/demo.cc \
	demo/simple_wave.cc
LOCAL_C_INCLUDES := \
	$(PROTOC_OUT_DIR) \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	external/protobuf/src \
	external/boringssl/include
LOCAL_SHARED_LIBRARIES := libpoco libspeech
LOCAL_CPPFLAGS := $(COMMON_CFLAGS) \
	-std=c++11
ifeq ($(PLATFORM_SDK_VERSION), 23)
LOCAL_CXX_STL := libc++
else ifeq ($(PLATFORM_SDK_VERSION), 22)
LOCAL_SHARED_LIBRARIES += libc++ libdl
else
LOCAL_SDK_VERSION := 14
LOCAL_NDK_STL_VARIANT := gnustl_static
endif
include $(BUILD_EXECUTABLE)
