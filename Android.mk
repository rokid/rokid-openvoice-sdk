LOCAL_PATH := $(call my-dir)

IGNORED_WARNINGS := -Wno-sign-compare -Wno-unused-parameter -Wno-sign-promo -Wno-error=return-type -Wno-error=non-virtual-dtor
COMMON_CFLAGS := \
	$(IGNORED_WARNINGS) \
	-DSPEECH_LOG_ANDROID \
	-DSPEECH_SDK_DETAIL_TRACE \
	-DSPEECH_STATISTIC \
	-DSSL_NON_VERIFY

include $(LOCAL_PATH)/core.mk

include $(LOCAL_PATH)/JavaLibrary.mk
