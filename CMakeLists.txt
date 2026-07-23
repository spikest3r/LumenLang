cmake_minimum_required(VERSION 3.13)
project(lumen)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SOURCES "src/*.cpp")

add_executable(lumen ${SOURCES})

target_include_directories(lumen PRIVATE include)

target_link_options(lumen PRIVATE
    "-lembind"
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
    "-sENVIRONMENT=web"
    "-sFORCE_FILESYSTEM=1"
    "-sEXPORTED_RUNTIME_METHODS=FS,wasmMemory"
    "-sALLOW_MEMORY_GROWTH=0"
    "-sINITIAL_MEMORY=67108864"
    "-sEXPORTED_FUNCTIONS=['_getLmnbin','_getLmnbinSize']"
)

set_target_properties(lumen PROPERTIES
    SUFFIX ".js"
)