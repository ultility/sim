cmake_minimum_required(VERSION 3.10)
PROJECT(sim C CXX)

set(SDL3_DIR "C:\\Lib\\SDL3-3.2.18\\x86_64-w64-mingw32")

include_directories(${SDL3_DIR}/include)
link_directories(${SDL3_DIR}/lib)


add_subdirectory(../flecs flecs_build)
add_subdirectory(src)
