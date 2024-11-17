cmake_minimum_required(VERSION 3.5.0)
project(HanabiGame VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compile Protobuf files
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/proto)  # Include the proto output directory

set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/newConnect.proto
    # Add more .proto files here
)

set(PROTO_OUT ${CMAKE_CURRENT_BINARY_DIR}/proto)

# Ensure the proto output directory exists
file(MAKE_DIRECTORY ${PROTO_OUT})

# Generate C++ source files from .proto files
foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    message("Processing proto file: ${PROTO_NAME}")
    set(PROTO_SRC ${PROTO_OUT}/${PROTO_NAME}.pb.cc)
    set(PROTO_HDR ${PROTO_OUT}/${PROTO_NAME}.pb.h)

    add_custom_command(
        OUTPUT ${PROTO_SRC} ${PROTO_HDR}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --cpp_out ${PROTO_OUT} -I ${CMAKE_CURRENT_SOURCE_DIR}/proto ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
    )
    list(APPEND PROTO_SRCS ${PROTO_SRC})
    list(APPEND PROTO_HDRS ${PROTO_HDR})
endforeach()

add_library(proto_files ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(proto_files ${Protobuf_LIBRARIES})

# Build the main executable
find_package(Boost 1.70 REQUIRED COMPONENTS system log)
add_compile_options(-Winvalid-pch -fsanitize=address)
add_link_options(-fsanitize=address)

add_executable(HanabiGame ${PROTO_SRCS} src/card.cpp src/deck.cpp src/game.cpp src/player.cpp src/server.cpp src/session.cpp src/sockets.cpp)
target_precompile_headers(HanabiGame PRIVATE inc/pch.h)
target_include_directories(HanabiGame PRIVATE inc ${PROTO_OUT} ${Boost_INCLUDE_DIRS})
target_link_libraries(HanabiGame ${Boost_LIBRARIES} ${Protobuf_LIBRARIES})

add_executable(HanabiTelegramClient ${PROTO_SRCS} src/telegram_client.cpp src/sockets.cpp)
target_precompile_headers(HanabiTelegramClient PRIVATE inc/pch.h)
target_include_directories(HanabiTelegramClient PRIVATE inc ${PROTO_OUT} ${Boost_INCLUDE_DIRS})
target_link_libraries(HanabiTelegramClient ${Boost_LIBRARIES} ${Protobuf_LIBRARIES})

include(CTest)
enable_testing() 