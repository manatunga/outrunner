cmake_minimum_required(VERSION 3.20)
project(s1_matmul LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED COMPONENTS glslc)

set(SHADER_NAMES
    matmul_naive
    matmul_tiled
    matmul_regblocked
)

set(SHADER_SPVS "")
foreach(SHADER_NAME ${SHADER_NAMES})
    set(SHADER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER_NAME}.comp")
    set(SHADER_SPV "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT "${SHADER_SPV}"
        COMMAND Vulkan::glslc "${SHADER_SRC}" -o "${SHADER_SPV}"
        DEPENDS "${SHADER_SRC}"
        COMMENT "Compiling ${SHADER_NAME}.comp to SPIR-V"
        VERBATIM
    )
    list(APPEND SHADER_SPVS "${SHADER_SPV}")
endforeach()

add_custom_target(s1_shaders DEPENDS ${SHADER_SPVS})

add_executable(s1_matmul main.cpp)
add_dependencies(s1_matmul s1_shaders)
target_link_libraries(s1_matmul PRIVATE Vulkan::Vulkan)
target_compile_definitions(s1_matmul PRIVATE SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}")

if(MSVC)
    target_compile_options(s1_matmul PRIVATE /W4)
else()
    target_compile_options(s1_matmul PRIVATE -Wall -Wextra -Wpedantic)
endif()