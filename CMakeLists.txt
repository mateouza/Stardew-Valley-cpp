cmake_minimum_required(VERSION 3.28)

project(StardewValley LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(course_support STATIC
    common/catedra/common_socket.cpp
    common/catedra/common_resolver.cpp
    common/catedra/common_resolvererror.cpp
    common/catedra/common_liberror.cpp
)

target_include_directories(course_support PUBLIC common/catedra)

target_compile_definitions(course_support PRIVATE _POSIX_C_SOURCE=200809L)

target_link_libraries(course_support PUBLIC Threads::Threads)

target_compile_options(course_support PRIVATE -Wall -Wextra -Wpedantic)