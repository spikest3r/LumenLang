cmake_minimum_required(VERSION 3.20)
project(lumen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

find_program(MOLD_LINKER mold)
if(MOLD_LINKER)
    add_link_options(-fuse-ld=mold)
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2>
)