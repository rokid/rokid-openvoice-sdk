
find_package(Protobuf REQUIRED)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS speech.proto)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(ROKIDSPEECH_PROTO_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)
add_library(rokidspeech.proto STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(rokidspeech.proto ${PROTOBUF_LIBRARIES})

