IGNORED_WARNINGS := -Wno-sign-compare -Wno-unused-parameter -Wno-sign-promo -Wno-error=return-type -Wno-error=non-virtual-dtor

include $(CLEAR_VARS)

LOCAL_MODULE := libspeech_common
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_REQUIRED_MODULES := rprotoc grpc_cpp_plugin

SPEECH_PROTO_FILE := $(LOCAL_PATH)/proto/speech.proto

PROTOC_OUT_DIR := $(call local-intermediates-dir)/gen
#PROTOC_OUT_DIR := $(local-generated-sources-dir)
GRPC_PLUGIN := $(HOST_OUT_EXECUTABLES)/grpc_cpp_plugin
RPROTOC := $(HOST_OUT_EXECUTABLES)/rprotoc

PROTOC_GEN_SRC := \
	$(PROTOC_OUT_DIR)/speech.grpc.pb.cc \
	$(PROTOC_OUT_DIR)/speech.pb.cc

COMMON_SRC := \
	src/common/speech_config.cc \
	src/common/speech_config.h \
	src/common/log.cc \
	src/common/log.h \
	src/common/speech_connection.cc \
	src/common/speech_connection.h \
	src/common/md5.h \
	src/common/md5.cc

LOCAL_C_INCLUDES := \
	$(PROTOC_OUT_DIR)

$(PROTOC_GEN_SRC): PRIVATE_CUSTOM_TOOL := $(RPROTOC) -I$(LOCAL_PATH)/proto --cpp_out=$(PROTOC_OUT_DIR) $(SPEECH_PROTO_FILE) && $(RPROTOC) -I$(LOCAL_PATH)/proto --grpc_out=$(PROTOC_OUT_DIR) --plugin=protoc-gen-grpc=$(GRPC_PLUGIN) $(SPEECH_PROTO_FILE)
$(PROTOC_GEN_SRC): $(SPEECH_PROTO_FILE)
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES := $(PROTOC_GEN_SRC)

LOCAL_SRC_FILES := $(COMMON_SRC)

LOCAL_CFLAGS := -std=c++11 $(IGNORED_WARNINGS) -DSPEECH_LOG_ANDROID -frtti
LOCAL_CXX_STL := libc++
LOCAL_SHARED_LIBRARIES := liblog libgrpc++ libprotobuf-rokid-cpp-full
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/src/common $(LOCAL_C_INCLUDES)

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libspeech_tts
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := $(call all-named-files-under,*.cc,src/tts)

LOCAL_C_INCLUDES := \
	$(PROTOC_OUT_DIR) \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS := -std=c++11 $(IGNORED_WARNINGS) -frtti
LOCAL_CXX_STL := libc++
LOCAL_SHARED_LIBRARIES := libspeech_common libgrpc++ libprotobuf-rokid-cpp-full

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libspeech_asr
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := $(call all-named-files-under,*.cc,src/asr)

LOCAL_C_INCLUDES := \
	$(PROTOC_OUT_DIR) \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS := -std=c++11 $(IGNORED_WARNINGS) -frtti
LOCAL_CXX_STL := libc++
LOCAL_SHARED_LIBRARIES := libspeech_common libgrpc++ libprotobuf-rokid-cpp-full

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libspeech_nlp
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := $(call all-named-files-under,*.cc,src/nlp)

LOCAL_C_INCLUDES := \
	$(PROTOC_OUT_DIR) \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS := -std=c++11 $(IGNORED_WARNINGS) -frtti
LOCAL_CXX_STL := libc++
LOCAL_SHARED_LIBRARIES := libspeech_common libgrpc++ libprotobuf-rokid-cpp-full

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libspeech
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := $(call all-named-files-under,*.cc,src/speech)

LOCAL_C_INCLUDES := \
	$(PROTOC_OUT_DIR) \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS := -std=c++11 $(IGNORED_WARNINGS) -frtti
LOCAL_CXX_STL := libc++
LOCAL_SHARED_LIBRARIES := libspeech_common libgrpc++ libprotobuf-rokid-cpp-full

include $(BUILD_SHARED_LIBRARY)
