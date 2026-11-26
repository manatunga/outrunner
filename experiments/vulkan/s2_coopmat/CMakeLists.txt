cmake_minimum_required(VERSION 3.20)
project(s2_coopmat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

add_executable(s2_coopmat main.cpp)
target_link_libraries(s2_coopmat PRIVATE Vulkan::Vulkan)

if(MSVC)
    target_compile_options(s2_coopmat PRIVATE /W4)
else()
    target_compile_options(s2_coopmat PRIVATE -Wall -Wextra -Wpedantic)
endif()
