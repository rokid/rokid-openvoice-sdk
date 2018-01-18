JNI_DIR := android/jni
JAVA_DIR := android/java
EXAMPLE_JAVA_DIR := android/example/java
EXAMPLE_JNI_DIR := android/example/jni

include $(CLEAR_VARS)
LOCAL_MODULE := librokid_speech_jni
LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src/common
LOCAL_CPPFLAGS := $(COMMON_FLAGS) -std=c++11
LOCAL_SHARED_LIBRARIES := libnativehelper libspeech liblog
ifeq ($(PLATFORM_SDK_VERSION), 22)
LOCAL_SHARED_LIBRARIES += libc++ libdl
LOCAL_C_INCLUDES += external/libcxx/include
else ifeq ($(PLATFORM_SDK_VERSION), 19)
LOCAL_SDK_VERSION := 14
LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_C_INCLUDES += libnativehelper/include/nativehelper
else
LOCAL_CXX_STL := libc++
endif
LOCAL_SRC_FILES := \
		$(JNI_DIR)/com_rokid_speech_Tts.cc \
		$(JNI_DIR)/com_rokid_speech_Speech.cc \
		$(JNI_DIR)/common.cc
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := rokid_speech
LOCAL_SRC_FILES := \
		$(JAVA_DIR)/com/rokid/speech/Tts.java \
		$(JAVA_DIR)/com/rokid/speech/Speech.java \
		$(JAVA_DIR)/com/rokid/speech/TtsCallback.java \
		$(JAVA_DIR)/com/rokid/speech/SpeechCallback.java \
		$(JAVA_DIR)/com/rokid/speech/TtsOptions.java \
		$(JAVA_DIR)/com/rokid/speech/SpeechOptions.java \
		$(JAVA_DIR)/com/rokid/speech/PrepareOptions.java \
		$(JAVA_DIR)/com/rokid/speech/GenericConfig.java
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS:=optional
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_PACKAGE_NAME := RKSpeechDemo
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES := \
		$(EXAMPLE_JAVA_DIR)/com/rokid/speech/example/TtsDemo.java \
		$(EXAMPLE_JAVA_DIR)/com/rokid/speech/example/TtsPlayerDemo.java \
		$(EXAMPLE_JAVA_DIR)/com/rokid/speech/example/SpeechDemo.java
LOCAL_STATIC_JAVA_LIBRARIES := rokid_speech
include $(BUILD_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := tts_sdk.json
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc
LOCAL_SRC_FILES := android/etc/tts_sdk.json
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := speech_sdk.json
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc
LOCAL_SRC_FILES := android/etc/speech_sdk.json
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := librokid_opus_jni
LOCAL_CPP_EXTENSION := .cc
LOCAL_CPPFLAGS := $(COMMON_FLAGS) -std=c++11
LOCAL_SHARED_LIBRARIES := libnativehelper liblog libspeech librkcodec
LOCAL_C_INCLUDES := \
	external/libopus/include \
	$(LOCAL_PATH)/src/common
ifeq ($(PLATFORM_SDK_VERSION), 22)
LOCAL_SHARED_LIBRARIES += libc++ libdl
LOCAL_C_INCLUDES += external/libcxx/include
else ifeq ($(PLATFORM_SDK_VERSION), 19)
LOCAL_SDK_VERSION := 14
LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_C_INCLUDES += libnativehelper/include/nativehelper
else
LOCAL_CXX_STL := libc++
endif
LOCAL_SRC_FILES := \
		$(JNI_DIR)/com_rokid_speech_Opus.cc
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := opus_player
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
	$(JAVA_DIR)/com/rokid/speech/Opus.java \
	$(JAVA_DIR)/com/rokid/speech/OpusPlayer.java
include $(BUILD_STATIC_JAVA_LIBRARY)
