cmake_minimum_required(VERSION 3.28)

project(StardewValley LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2pp REQUIRED)

add_library(course_support STATIC
    common/common_socket.cpp
    common/common_resolver.cpp
    common/common_resolvererror.cpp
    common/common_liberror.cpp
)

target_include_directories(course_support PUBLIC common/catedra)

target_compile_definitions(course_support PRIVATE _POSIX_C_SOURCE=200809L)

target_link_libraries(course_support PUBLIC Threads::Threads)

target_compile_options(course_support PRIVATE -Wall -Wextra -Wpedantic)

add_executable(stardew_client
    sdl/main_test.cpp
    sdl/renderer.cpp
    sdl/client_state.cpp
    sdl/input_handler.cpp
)

target_link_libraries(stardew_client PRIVATE
    course_support
    SDL2pp::SDL2pp
)

target_compile_options(stardew_client PRIVATE -Wall -Wextra -Wpedantic)