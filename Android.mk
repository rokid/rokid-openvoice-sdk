LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/core.mk

include $(LOCAL_PATH)/JavaLibrary.mk

include $(CLEAR_VARS)
LOCAL_MODULE := roots.pem
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc
LOCAL_SRC_FILES := etc/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)
