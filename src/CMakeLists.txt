# ------------------------------------
# Router simulator makefile
# ------------------------------------

cmake_minimum_required(VERSION 3.18)
project(RouterSimulator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -ggdb")

set(BUILD_DIR "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BUILD_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BUILD_DIR}/lib") 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BUILD_DIR}")

include_directories(include)
file(GLOB_RECURSE ROUTER_LIB_SRC
    "lib/*.cpp")

file(GLOB BIN_SRC "bin/*.cpp")
add_library(Router STATIC ${ROUTER_LIB_SRC})
target_include_directories(Router PUBLIC include)

foreach(bin_file ${BIN_SRC})
    get_filename_component(bin_name ${bin_file} NAME_WE)
    add_executable(${bin_name} ${bin_file})
    target_link_libraries(${bin_name} PRIVATE Router)
endforeach()
