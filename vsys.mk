local-path := $(call my-dir)

include $(clear-vars)
local.module := speech_pb
local.src-paths := proto
local.excludes := proto/speech_types.proto
local.inc-dirs:= proto
include $(build-pb-cpp)

include $(clear-vars)
local.module := speech
local.ndk-script := $(local-path)/ndk.mk
local.pb-cpp-modules := speech_pb
local.ndk-modules := uWS protobuf
include $(build-ndk-module)

include $(clear-vars)
local.module := rokid_speech
local.src-paths := android/java
include $(build-java-lib)
