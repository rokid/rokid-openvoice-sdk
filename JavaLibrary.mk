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
LOCAL_SHARED_LIBRARIES := libnativehelper libspeech librlog
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
LOCAL_MODULE := librokid_opus_jni
LOCAL_CPP_EXTENSION := .cc
LOCAL_CPPFLAGS := $(COMMON_FLAGS) -std=c++11
LOCAL_SHARED_LIBRARIES := libnativehelper librlog libspeech librkcodec
LOCAL_C_INCLUDES := \
	external/libopus/include \
	$(LOCAL_PATH)/src/common
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
