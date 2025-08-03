cmake_minimum_required(VERSION 3.10)
PROJECT(sim C CXX)

set(SDL3_DIR "C:\\Lib\\SDL3-3.2.18\\x86_64-w64-mingw32")
set(SDL3_IMAGE_DIR "C:\\Lib\\SDL3_image-3.2.4\\x86_64-w64-mingw32")
set(PARASAIL_DIR "C:\\Lib\\parasail-2.6.2-win64-v140")
include_directories(${SDL3_DIR}/include)
link_directories(${SDL3_DIR}/lib)
include_directories(${SDL3_IMAGE_DIR}/include)
link_directories(${SDL3_IMAGE_DIR}/lib)
include_directories(${PARASAIL_DIR}/include)
link_directories(${PARASAIL_DIR}/lib)

add_subdirectory(../flecs flecs_build)
add_subdirectory(src)
message(${CMAKE_CURRENT_SOURCE_DIR}/images)
add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${PROJECT_SOURCE_DIR}/bin/assets
)
add_dependencies(${PROJECT_NAME} copy_assets)