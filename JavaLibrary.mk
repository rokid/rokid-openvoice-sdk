JNI_DIR := android/jni
JAVA_DIR := android/java
EXAMPLE_JAVA_DIR := android/example/java
EXAMPLE_JNI_DIR := android/example/jni

include $(CLEAR_VARS)

LOCAL_MODULE := librokid_tts_jni
LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS := -frtti

LOCAL_SRC_FILES := \
		$(JNI_DIR)/com_rokid_speech_Tts.cc

LOCAL_SHARED_LIBRARIES := libnativehelper libspeech_tts libspeech_common liblog

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := rokid_tts

LOCAL_SRC_FILES := \
		$(JAVA_DIR)/com/rokid/speech/Tts.java \
		$(JAVA_DIR)/com/rokid/speech/TtsCallback.java
LOCAL_STATIC_JAVA_LIBRARIES := fastjson-android

include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := librokid_speech_jni
LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS := -frtti

LOCAL_SRC_FILES := \
		$(JNI_DIR)/com_rokid_speech_Speech.cc

LOCAL_SHARED_LIBRARIES := libnativehelper libspeech libspeech_common liblog

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := rokid_speech

LOCAL_SRC_FILES := \
		$(JAVA_DIR)/com/rokid/speech/Speech.java \
		$(JAVA_DIR)/com/rokid/speech/SpeechCallback.java
LOCAL_STATIC_JAVA_LIBRARIES := fastjson-android

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
LOCAL_STATIC_JAVA_LIBRARIES := rokid_tts rokid_speech

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
