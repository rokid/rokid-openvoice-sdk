LOCAL_PATH := $(call my-dir)

IGNORED_WARNINGS := -Wno-sign-compare -Wno-unused-parameter -Wno-sign-promo -Wno-error=return-type -Wno-error=non-virtual-dtor
COMMON_CFLAGS := \
	$(IGNORED_WARNINGS) \
	-DSPEECH_LOG_ANDROID \
	-DSPEECH_SDK_STREAM_QUEUE_TRACE \
	-DSPEECH_SDK_DETAIL_TRACE

include $(LOCAL_PATH)/core.mk

include $(LOCAL_PATH)/JavaLibrary.mk

include $(CLEAR_VARS)
LOCAL_MODULE := roots.pem
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc
LOCAL_SRC_FILES := etc/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
