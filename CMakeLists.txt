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

# Compile Protobuf files
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/common/proto) # Include the proto output directory
set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/common/proto/newConnect.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/common/proto/ack.proto
)
set(PROTO_OUT ${CMAKE_CURRENT_BINARY_DIR}/common/proto)
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

# Create a static library for the common code
find_package(Boost 1.70 REQUIRED COMPONENTS system log)
add_library(HanabiCommon STATIC ${PROTO_SRCS} common/src/boost_asio_impl.cpp common/src/sockets.cpp)
target_precompile_headers(HanabiCommon PRIVATE common/inc/pch.h)
target_include_directories(HanabiCommon PRIVATE common/inc ${PROTO_OUT})
target_compile_definitions(HanabiCommon PRIVATE BOOST_ASIO_SEPARATE_COMPILATION)
target_link_libraries(HanabiCommon ${Boost_LIBRARIES} ${Protobuf_LIBRARIES})

# Function to automatically add source files based on directory
function(add_sources_from_directory target directory)
    file(GLOB_RECURSE SOURCES ${directory}/*.cpp)
    target_sources(${target} PRIVATE ${SOURCES})
endfunction()

# Build the main executable
add_executable(HanabiGame)
add_sources_from_directory(HanabiGame hanabiGame/src)
target_precompile_headers(HanabiGame PRIVATE common/inc/pch.h)
target_include_directories(HanabiGame PRIVATE hanabiGame/inc ${PROTO_OUT})
target_link_libraries(HanabiGame HanabiCommon)
target_compile_definitions(HanabiGame PRIVATE BOOST_ASIO_SEPARATE_COMPILATION)

# Build the Telegram client executable
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL)
include_directories(/usr/local/include ${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIR})
if(CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    add_definitions(-DHAVE_CURL)
endif()

add_executable(HanabiTelegramClient)
add_sources_from_directory(HanabiTelegramClient hanabiTelegramClient/src)
target_precompile_headers(HanabiTelegramClient PRIVATE common/inc/pch.h hanabiTelegramClient/inc/telegram_client_pch.h)
target_include_directories(HanabiTelegramClient PRIVATE hanabiTelegramClient/inc ${PROTO_OUT})
target_link_libraries(HanabiTelegramClient HanabiCommon /usr/local/lib/libTgBot.a ${OPENSSL_LIBRARIES} ${CURL_LIBRARIES})
target_compile_definitions(HanabiTelegramClient PRIVATE BOOST_ASIO_SEPARATE_COMPILATION)
