cmake_minimum_required(VERSION 3.15.0)
project(Cpp_Design_Patterns VERSION 0.1.0)

add_compile_options(
    -std=c++17
)

# Clang Debug模式下加入-fstandalone-debug方便调试
if (CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug")
    AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    add_compile_options(
        -fstandalone-debug
    )
endif()

add_executable(
    Cpp_Design_Patterns main.cpp
)
