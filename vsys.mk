local-path := $(call my-dir)

include $(clear-vars)
local.module := speech
local.ndk-script := $(local-path)/ndk.mk
local.ndk-modules := uWS mutils boringssl
include $(build-ndk-module)

include $(clear-vars)
local.module := rokid_speech
local.src-paths := android/java
include $(build-java-lib)

include $(clear-vars)
local.module := phony-speech
local.ndk-modules := speech
include $(build-executable)

include $(clear-vars)
local.module := speech-java-example
local.jar-modules := rokid_speech
local.manifest := AndroidManifest.xml
local.src-paths := android/example/java
include $(build-java-app)
