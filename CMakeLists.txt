CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(rokidspeech)
set(VERSION 0.1)

unset(CMAKE_SKIP_RPATH CACHE)
set(CMAKE_SKIP_RPATH TRUE)
option(BUILD_DEMO "Build demo test" OFF)
option(TEST_MP3 "test tts mp3 codec" OFF)
option(BUILD_DEBUG "debug or release" OFF)
option(SPEECH_STATISTIC "speech statistic" ON)
option(NO_STD_STEADY_CLOCK "no std steady_clock" OFF)
option(ROKID_UPLOAD_TRACE "runtime exception upload to server" OFF)
option(UWS_THREADSAFE "uWS threadsafe" ON)

# Configuration fails on < 3.9 if only C++ is enabled (TestBigEndian macro)
if(NOT CMAKE_VERSION VERSION_LESS 3.9.0)
    set(LANG CXX)
endif()


# CMake policies: enable MACOSX_RPATH by default
if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

# Don't treat imported targets with :: as files
if(POLICY CMP0028)
    cmake_policy(SET CMP0028 NEW)
endif()

#SET(CMAKE_MACOSX_RPATH 1)

#############################################################################
# Turning on this flag tells cmake to emit a compile_commands.json file.
# This file can be used to load compilation flags into YCM. See here for more
# details: http://clang.llvm.org/docs/JSONCompilationDatabase.html
set( CMAKE_EXPORT_COMPILE_COMMANDS 1 )

#############################################################################

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
  set(COMPILER_IS_CLANG true)

  # The Travis CI build machines don't have libc++ installed
  if (NOT DEFINED ENV{TRAVIS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++" )
  endif()

  # Ninja will by default prevent Clang from outputting diagnostics in color
  if (CMAKE_GENERATOR STREQUAL "Ninja")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics" )
  endif()
endif()

#option(BUILD_DEPRECATED "Include deprecated API in the build" ON)

# Use folders for nice tree in Visual Studio and XCode
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(CMakeDependentOption)

if(UNIX)
    # Both APPLE and UNIX are defined on OSX
    if(APPLE)
	option(TESTSUITE_TARGET_XCTEST "Build TestSuite tests compatible with Xcode XCTest" OFF)
	find_package(Threads REQUIRED)
    endif()
endif()

include(${CUSTOM_CMAKE_MODULES}/common.mk)

include(CheckSymbolExists)
include(CMakePushCheckState)
include(GNUInstallDirs)

if(NOT CMAKE_INSTALL_RPATH AND CMAKE_INSTALL_PREFIX)
	set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
	message(STATUS "Setting install rpath to ${CMAKE_INSTALL_RPATH}")
endif()

if (EnableOpus STREQUAL "static")
set(opus_find_args STATIC_LIBS opus)
elseif (EnableOpus STREQUAL "shared")
set(opus_find_args SHARED_LIBS opus)
endif()

findPackage(UWS REQUIRED
	HINTS ${UWSPrefix}
	INC_PATH_SUFFIX include/uWS
	HEADERS Hub.h
	SHARED_LIBS uWS
)
findPackage(RLog REQUIRED
	HINTS ${RLogPrefix}
	INC_PATH_SUFFIX include/log
	HEADERS rlog.h
	SHARED_LIBS rlog
)
if (opus_find_args)
findPackage(Opus REQUIRED
	HINTS ${OpusPrefix}
	INC_PATH_SUFFIX include/opus
	HEADERS opus.h
	${opus_find_args}
)
endif()
findPackage(OpenSSL REQUIRED
	HINTS ${OpenSSLPrefix}
	HEADERS openssl/ssl.h
	SHARED_LIBS ssl crypto
)
findPackage(ZLIB REQUIRED
	HINTS ${ZLIBPrefix}
	HEADERS zlib.h
	SHARED_LIBS z
)
if (ROKID_UPLOAD_TRACE)
findPackage(Poco REQUIRED
	HINTS ${PocoPrefix}
	HEADERS Poco/Net/HTTPRequest.h
	STATIC_LIBS poco_static
)
findPackage(Jsonc REQUIRED
	HINTS ${JsoncPrefix}
	HEADERS json.h
	INC_PATH_SUFFIX jsonc
	STATIC_LIBS jsonc_static
)
endif()
# set(IGNORED_WARNINGS "-Wno-sign-compare -Wno-unused-parameter -Wno-sign-promo -Wno-error=return-type -Wno-error=non-virtual-dtor")

set(COMMON_CXXFLAGS "-std=c++11 -pthread -fPIC -D__STDC_FORMAT_MACROS")
set(COMMON_CFLAGS "-pthread -fPIC -D__STDC_FORMAT_MACROS")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXXFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_CFLAGS}")
if (CROSS_COMPILE_CXXFLAGS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CROSS_COMPILE_CXXFLAGS}")
endif()
if (BUILD_DEBUG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
elseif(CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -s")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -s")
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -s")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -s")
endif()
if (SPEECH_STATISTIC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSPEECH_STATISTIC")
endif()
set(LogLevel 2)
if (LOG_ENABLED STREQUAL "verbose")
	set(LogLevel 0)
elseif (LOG_ENABLED STREQUAL "debug")
	set(LogLevel 1)
elseif (LOG_ENABLED STREQUAL "info")
	set(LogLevel 2)
elseif (LOG_ENABLED STREQUAL "warn")
	set(LogLevel 3)
elseif (LOG_ENABLED STREQUAL "error")
	set(LogLevel 4)
endif()
if (NO_STD_STEADY_CLOCK)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_STD_STEADY_CLOCK")
	set(ALTCHRONO_SRCS src/common/alt_chrono.cc src/common/alt_chrono.h)
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DROKID_LOG_ENABLED=${LogLevel}")
if (UWS_THREADSAFE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUWS_THREADSAFE")
endif()
# workaround for g++ threading bug
if (NOT COMPILER_IS_CLANG)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed")
endif()
if (ROKID_UPLOAD_TRACE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DROKID_UPLOAD_TRACE")
endif()

set (COMMON_INCLUDE_DIRS include src/common)
# target library 'rkcodec'
if (Opus_LIBRARIES)
	set (CODEC_SOURCES
		src/codec/rkcodec.cc
		include/rkcodec.h
	)
	add_library(rkcodec SHARED ${CODEC_SOURCES})
	target_include_directories(rkcodec
	PRIVATE
		${COMMON_INCLUDE_DIRS}
		${RLog_INCLUDE_DIRS}
	PUBLIC
		${Opus_INCLUDE_DIRS}
	)
	target_link_libraries(rkcodec
		${Opus_LIBRARIES}
		${RLog_LIBRARIES}
	)
	set(speech_extra_cflags "-DHAS_OPUS_CODEC")
	set(speech_extra_libs rkcodec)
	install (TARGETS rkcodec LIBRARY DESTINATION lib)
	install (FILES include/rkcodec.h DESTINATION include/speech)
endif()

# target library 'speech'
file(GLOB_RECURSE COMMON_SOURCES 
	src/common/*.h
	src/common/*.cc
	src/common/*.c
)
file (GLOB_RECURSE SPEECH_SOURCES
	src/speech/*.h
	src/speech/*.cc
)
file (GLOB_RECURSE TTS_SOURCES
	src/tts/*.h
	src/tts/*.cc
)
set (PROTO_SRCS
	nanopb-gen/auth.pb.c
	nanopb-gen/speech.pb.c
	nanopb-gen/tts.pb.c
	nanopb-gen/speech_types.pb.c
)
set (NANOPB_SRCS
	nanopb/pb_common.c
	nanopb/pb_decode.c
	nanopb/pb_encode.c
)
set (NANOPB_INCLUDE_DIRS
	nanopb-gen
	nanopb
)
add_library(speech SHARED
	${COMMON_SOURCES}
	${SPEECH_SOURCES}
	${TTS_SOURCES}
	${PROTO_SRCS}
	${NANOPB_SRCS}
	${ALTCHRONO_SRCS}
)
target_include_directories(speech PRIVATE
	${COMMON_INCLUDE_DIRS}
	${UWS_INCLUDE_DIRS}
	${OpenSSL_INCLUDE_DIRS}
	${NANOPB_INCLUDE_DIRS}
	${RLog_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
)
target_compile_options(speech PRIVATE
	${speech_extra_cflags}
)
target_link_libraries(speech
	${UWS_LIBRARIES}
	${OpenSSL_LIBRARIES}
	${RLog_LIBRARIES}
	${ZLIB_LIBRARIES}
	${speech_extra_libs}
	-lresolv
)
if (ROKID_UPLOAD_TRACE)
	target_include_directories(speech PRIVATE
		${Poco_INCLUDE_DIRS}
		${Jsonc_INCLUDE_DIRS}
	)
	target_link_libraries(speech
		${Poco_LIBRARIES}
		${Jsonc_LIBRARIES}
	)
endif()
install(TARGETS speech
	LIBRARY DESTINATION lib
)
set(HEADER_FILES
	include/speech.h
	include/tts.h
	include/speech_common.h
)
install(FILES ${HEADER_FILES}
	DESTINATION include/speech
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

#
# build demo
#
# file(GLOB_RECURSE DEMO_SOURCES demo/*.h demo/*.cc)
if (BUILD_DEMO)
	set(DEMO_SOURCES
		demo/demo.cc
		demo/simple_wave.cc
		demo/simple_wave.h
		demo/speech_stress_test.cc
		demo/speech_stress_test.h
		demo/clargs.cc
		demo/clargs.h
		demo/test_utils.cc
		demo/test_utils.h
		demo/speech_release_test.cc
		demo/speech_release_test.h
		demo/speech_base_test.cc
		demo/speech_base_test.h
	)
	add_executable(demo ${DEMO_SOURCES})
	target_include_directories(demo PRIVATE
		${COMMON_INCLUDE_DIRS}
		${RLog_INCLUDE_DIRS}
		${CMAKE_CURRENT_BINARY_DIR}
	)
	target_link_libraries(demo
		speech
		-Wl,-rpath,${CMAKE_INSTALL_PREFIX}/lib
	)
	if (Opus_LIBRARIES)
		target_compile_definitions(demo PRIVATE "HAS_OPUS_CODEC")
		target_link_libraries(demo rkcodec)
	endif()
	if (TEST_MP3)
		target_compile_definitions(demo PRIVATE "TEST_MP3")
	endif()

	add_executable(nanopb-demo
		${PROTO_SRCS}
		${NANOPB_SRCS}
		demo/nanopb_demo.cc
		src/common/nanopb_encoder.cc
	)
	target_include_directories(nanopb-demo PRIVATE
		nanopb
		nanopb-gen
		src/common
	)

if (ROKID_UPLOAD_TRACE)
	add_executable(trace-demo
		demo/trace_demo.cc
	)
	target_include_directories(trace-demo PRIVATE
		src/common
		${Poco_INCLUDE_DIRS}
	)
	target_link_libraries(trace-demo
		speech
		-Wl,-rpath,${CMAKE_INSTALL_PREFIX}/lib
	)
endif(ROKID_UPLOAD_TRACE)

endif(BUILD_DEMO)
