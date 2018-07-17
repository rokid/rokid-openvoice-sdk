include $(CLEAR_VARS)

LOCAL_MODULE := libspeech
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	$(LOCAL_PATH)/nanopb \
	$(LOCAL_PATH)/nanopb-gen

COMMON_SRC := \
	src/common/log_plugin.cc \
	src/common/speech_connection.cc \
	src/common/nanopb_encoder.cc \
	src/common/nanopb_decoder.cc

TTS_SRC := \
	src/tts/tts_impl.cc

SPEECH_SRC := \
	src/speech/speech_impl.cc

PB_SRC := \
	nanopb-gen/auth.pb.c \
	nanopb-gen/speech_types.pb.c \
	nanopb-gen/speech.pb.c \
	nanopb-gen/tts.pb.c \
	nanopb/pb_common.c \
	nanopb/pb_decode.c \
	nanopb/pb_encode.c

LOCAL_SRC_FILES := \
	$(COMMON_SRC) \
	$(TTS_SRC) \
	$(SPEECH_SRC) \
	$(PB_SRC)

LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_CFLAGS := $(COMMON_CFLAGS)
LOCAL_CPPFLAGS := -std=c++11
LOCAL_SHARED_LIBRARIES := libuWS librlog
LOCAL_LDLIBS := -llog -lz
LOCAL_C_INCLUDES += $(DEPS_DIR)/boringssl/include
LOCAL_LDLIBS += $(DEPS_DIR)/boringssl/libs/$(TARGET_ABI_NAME)/libcrypto_static.a
ifeq ($(PLATFORM_SDK_VERSION), 19)
LOCAL_CFLAGS += -D__STDC_FORMAT_MACROS
endif
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := librkcodec
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
	src/codec/rkcodec.cc \
	include/rkcodec.h
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	$(DEPS_DIR)/opus/include
LOCAL_SHARED_LIBRARIES := librlog
LOCAL_CPPFLAGS := -std=c++11
LOCAL_LDLIBS := $(DEPS_DIR)/opus/libs/$(TARGET_ABI_NAME)/libopus.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
include $(BUILD_SHARED_LIBRARY)
