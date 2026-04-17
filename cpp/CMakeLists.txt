cmake_minimum_required(VERSION 3.20)
project(mirror CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable CMake's automatic rpath handling — we set it manually below.
set(CMAKE_SKIP_RPATH TRUE)

add_executable(mirror main.cpp
        types.h
        1_enum_string.h
        2_json_serialize.h
        3_describe.h
        4_generic_equal.h)

# Use the p2996 fork's libc++ (which includes <meta>) instead of the system one.
# CMAKE_CXX_COMPILER should point to the p2996 clang++ binary; its libc++ lives
# alongside it in the same install/build tree.
get_filename_component(CLANG_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(CLANG_ROOT "${CLANG_BIN_DIR}" DIRECTORY)

target_compile_options(mirror PRIVATE
    -freflection -fexpansion-statements
    -nostdinc++ -isystem "${CLANG_ROOT}/include/c++/v1")

target_link_options(mirror PRIVATE
    -L "${CLANG_ROOT}/lib"
    -lc++ -lc++abi
    "-Wl,-rpath,${CLANG_ROOT}/lib")
