cmake_minimum_required(VERSION 3.5.0)
project(HanabiGame VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Boost 1.70 REQUIRED COMPONENTS filesystem system log)

add_compile_options(-Winvalid-pch -fsanitize=address)
add_link_options(-fsanitize=address)

add_executable(HanabiGame card.cpp deck.cpp game.cpp player.cpp server.cpp session.cpp sockets.cpp)
target_precompile_headers(HanabiGame PRIVATE pch.h)
target_include_directories(HanabiGame PRIVATE include ${Boost_INCLUDE_DIRS})
target_link_libraries(HanabiGame ${Boost_LIBRARIES})

include(CTest)
enable_testing()