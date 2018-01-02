JNI_DIR := android/jni

include $(CLEAR_VARS)
LOCAL_MODULE := librokid_speech_jni
LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common \
	$(DEPS_DIR)/nativehelper/include/nativehelper
LOCAL_CPPFLAGS := $(COMMON_FLAGS) -std=c++11
LOCAL_SHARED_LIBRARIES := libspeech
LOCAL_LDLIBS := -L$(DEPS_DIR)/nativehelper/libs -lnativehelper -llog
LOCAL_SRC_FILES := \
		$(JNI_DIR)/com_rokid_speech_Tts.cc \
		$(JNI_DIR)/com_rokid_speech_Speech.cc \
		$(JNI_DIR)/common.cc
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := librokid_opus_jni
LOCAL_CPP_EXTENSION := .cc
LOCAL_CPPFLAGS := $(COMMON_FLAGS) -std=c++11
LOCAL_SHARED_LIBRARIES := libspeech librkcodec
LOCAL_LDLIBS := \
	-L$(DEPS_DIR)/nativehelper/libs -lnativehelper \
	-llog
# ifeq ($(PLATFORM_SDK_VERSION),19)
# LOCAL_LDLIBS += -L$(DEPS_DIR)/opus/libs/19 -lopus
# else
# LOCAL_LDLIBS += -L$(DEPS_DIR)/opus/libs/23 -lopus
# endif
LOCAL_C_INCLUDES := \
	$(DEPS_DIR)/opus/include \
	$(DEPS_DIR)/nativehelper/include/nativehelper \
	$(LOCAL_PATH)/src/common
LOCAL_SRC_FILES := \
		$(JNI_DIR)/com_rokid_speech_Opus.cc
include $(BUILD_SHARED_LIBRARY)
