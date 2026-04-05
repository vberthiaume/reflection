cmake_minimum_required(VERSION 3.20)
project(reflection_demo CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(reflection_demo main.cpp)

target_compile_options(reflection_demo PRIVATE -freflection -fexpansion-statements)

# Use the p2996 fork's libc++ (which includes <meta>) instead of the system one.
# CMAKE_CXX_COMPILER should point to the p2996 clang++ binary; its libc++ lives
# alongside it in the same install/build tree.
get_filename_component(CLANG_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(CLANG_ROOT "${CLANG_BIN_DIR}" DIRECTORY)

target_compile_options(reflection_demo PRIVATE -nostdinc++ -isystem "${CLANG_ROOT}/include/c++/v1")
target_link_directories(reflection_demo PRIVATE "${CLANG_ROOT}/lib")
target_link_libraries(reflection_demo PRIVATE c++ c++abi)
target_link_options(reflection_demo PRIVATE "-Wl,-rpath,${CLANG_ROOT}/lib")
