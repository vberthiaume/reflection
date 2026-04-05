cmake_minimum_required(VERSION 3.20)
project(reflection_demo CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(reflection_demo main.cpp)

# Bloomberg's clang-p2996 fork needs -freflection
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(reflection_demo PRIVATE -freflection)
endif()
