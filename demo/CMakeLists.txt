
find_package(OpenSSL REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/common)
include_directories(${Poco_INCLUDE_DIRS})
include_directories(${OpenSSL_INCLUDE_DIR})
file(GLOB_RECURSE SOURCES *.h *.cc)
add_executable(rokidspeech.demo common.h demo.cc asr_demo.cc speech_demo.cc tts_demo.cc)
target_link_libraries(rokidspeech.demo speech ${Poco_LIBRARIES} ${OpenSSL_LIBRAIES})

