cmake_minimum_required(VERSION 3.3)
project(libvgm)

if (NOT MSCV)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

file(GLOB H_FILES *.h)
file(GLOB C_FILES *.c)

add_library(libserial ${C_FILES} ${H_FILES})
target_include_directories(libserial PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
