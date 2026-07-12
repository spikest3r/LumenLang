cmake_minimum_required(VERSION 3.13)
project(lumen)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SOURCES "src/*.cpp")

add_executable(lumen ${SOURCES})

target_include_directories(lumen PRIVATE include)

set_target_properties(lumen PROPERTIES
    LINK_FLAGS "-lembind -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web -sALLOW_MEMORY_GROWTH=1 -sFORCE_FILESYSTEM=1 -sEXPORTED_RUNTIME_METHODS=FS -sDISABLE_EXCEPTION_CATCHING=0"
    SUFFIX ".js"
)