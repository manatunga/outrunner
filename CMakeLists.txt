
cmake_minimum_required(VERSION 3.20)

project(Outrunner
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# CPU reference implementation

add_library(outrunner_cpu STATIC
    src/cpu/vector_add.cpp
    src/cpu/matmul.cpp
)

target_include_directories(outrunner_cpu
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)


# Tests

enable_testing()

add_executable(test_vector_add
    tests/test_vector_add.cpp
)

target_link_libraries(test_vector_add
    PRIVATE outrunner_cpu
)

add_test(
    NAME vector_add
    COMMAND test_vector_add
)


add_executable(test_matmul
    tests/test_matmul.cpp
)

target_link_libraries(test_matmul
    PRIVATE outrunner_cpu
)

add_test(
    NAME matmul
    COMMAND test_matmul
)
