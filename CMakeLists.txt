CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
project(rokidspeech)
set(VERSION 0.1)

option(BUILD_DEMO "Build demo" ON)

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

option(BUILD_STATIC "Build static libraries (default are shared)" OFF)

include(CMakeDependentOption)


if(UNIX)
    # Both APPLE and UNIX are defined on OSX
    if(APPLE)
	option(TESTSUITE_TARGET_XCTEST "Build TestSuite tests compatible with Xcode XCTest" OFF)
	find_package(Threads REQUIRED)
    endif()
endif()


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CheckSymbolExists)
include(CMakePushCheckState)
include(GNUInstallDirs)

if(NOT CMAKE_INSTALL_RPATH AND CMAKE_INSTALL_PREFIX)
	set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
	message(STATUS "Setting install rpath to ${CMAKE_INSTALL_RPATH}")
endif()

find_package(Protobuf REQUIRED)
# Needed for FindPoco
#find_package(Poco REQUIRED Net Crypto  NetSSL_OpenSSL)
find_package(Poco REQUIRED Net Crypto NetSSL)

set(IGNORED_WARNINGS "-Wno-sign-compare -Wno-unused-parameter -Wno-sign-promo -Wno-error=return-type -Wno-error=non-virtual-dtor")

set(COMMON_CFLAGS "-DSPEECH_SDK_STREAM_QUEUE_TRACE \
  -DSPEECH_SDK_DETAIL_TRACE \
  -std=gnu++11 -fPIC -frtti -fexceptions")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IGNORED_WARNINGS} ${COMMON_CFLAGS}")

#
# Build gennerated protobuf cpp files.
#
add_subdirectory(proto)

# install include files.
set(HEADER_FILES
	asr.h
	speech.h
	tts.h
	speech_types.h
)

set(ROKIDSPEECH_HEADER_FILES)
foreach(HEADER_FILE ${HEADER_FILES})
	list(APPEND ROKIDSPEECH_HEADER_FILES "${CMAKE_CURRENT_LIST_DIR}/include/${HEADER_FILE}")
endforeach()
set(ROKIDSPEECH_HEADER_FILES ${ROKIDSPEECH_HEADER_FILES} PARENT_SCOPE)

install(FILES ${ROKIDSPEECH_HEADER_FILES}
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/speech
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

#
# speech sdk source dir.
#
include_directories(${ROKIDSPEECH_PROTO_INCLUDE_DIR})
include_directories(include)

add_subdirectory(src)

#
# build demo
#
if(BUILD_DEMO)
  add_subdirectory(demo)
endif(BUILD_DEMO)

