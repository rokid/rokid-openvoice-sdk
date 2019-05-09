local-path := $(call my-dir)

include $(clear-vars)
local.module := speech
local.ndk-script := $(local-path)/ndk.mk
local.ndk-modules := uWS mutils
include $(build-ndk-module)

include $(clear-vars)
local.module := rokid_speech
local.src-paths := android/java
include $(build-java-lib)

include $(clear-vars)
local.module := phony-speech
local.ndk-modules := speech
include $(build-executable)
