cmake_minimum_required(VERSION 3.5.0)
project(HanabiGame VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Winvalid-pch -fsanitize=address -fsanitize=undefined)
add_link_options(-fsanitize=address -fsanitize=undefined)

# ccache support
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()

# Find required packages
find_package(Boost 1.70 REQUIRED COMPONENTS system log)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL)
include_directories(/usr/local/include ${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIR})
if(CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    add_definitions(-DHAVE_CURL)
endif()

# == Build the static library for the precompiled header
add_library(CommonPCH INTERFACE)
target_precompile_headers(CommonPCH INTERFACE common/inc/pch.h)
target_include_directories(CommonPCH INTERFACE common/inc)

# == Build the Common static libary ==
# Compile Protobuf files
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/proto) # Include the proto output directory
include_directories(hanabi_game/inc common/inc tg_client/inc)
set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/common/proto/newConnect.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/common/proto/ack.proto
)
set(PROTO_OUT ${CMAKE_CURRENT_BINARY_DIR}/proto)
file(MAKE_DIRECTORY ${PROTO_OUT})

# Generate C++ source files from .proto files
foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(PROTO_SRC ${PROTO_OUT}/${PROTO_NAME}.pb.cc)
    set(PROTO_HDR ${PROTO_OUT}/${PROTO_NAME}.pb.h)

    add_custom_command(
        OUTPUT ${PROTO_SRC} ${PROTO_HDR}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --cpp_out ${PROTO_OUT} -I ${CMAKE_CURRENT_SOURCE_DIR}/common/proto ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
    )
    list(APPEND PROTO_SRCS ${PROTO_SRC})
    list(APPEND PROTO_HDRS ${PROTO_HDR})
endforeach()

add_library(HanabiCommon STATIC ${PROTO_SRCS} 
    common/src/sockets.cpp 
    common/src/boost_asio_impl.cpp
)
target_compile_definitions(HanabiCommon PRIVATE BOOST_ASIO_SEPARATE_COMPILATION)
target_include_directories(HanabiCommon PRIVATE common/inc ${PROTO_OUT})
target_link_libraries(HanabiCommon ${Boost_LIBRARIES} ${Protobuf_LIBRARIES})

# == Build the Hanabi Game executable ==
add_executable(HanabiGame 
    hanabi_game/src/card.cpp
    hanabi_game/src/deck.cpp
    hanabi_game/src/game.cpp
    hanabi_game/src/player.cpp
    hanabi_game/src/server.cpp
    hanabi_game/src/session.cpp
)

target_compile_definitions(HanabiGame PRIVATE BOOST_ASIO_SEPARATE_COMPILATION)
target_include_directories(HanabiGame PRIVATE hanabi_game/inc ${PROTO_OUT})
target_link_libraries(HanabiGame CommonPCH HanabiCommon)

# == Build the Telegram Client executable ==
add_executable(HanabiTelegramClient 
    tg_client/src/telegram_client.cpp 
    tg_client/src/client_entry.cpp 
    tg_client/src/telegram_keyboard.cpp
)

target_compile_definitions(HanabiTelegramClient PRIVATE BOOST_ASIO_SEPARATE_COMPILATION)
target_include_directories(HanabiTelegramClient PRIVATE tg_client/inc ${PROTO_OUT})
target_link_libraries(HanabiTelegramClient CommonPCH HanabiCommon /usr/local/lib/libTgBot.a ${OPENSSL_LIBRARIES} ${CURL_LIBRARIES})
