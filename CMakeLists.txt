cmake_minimum_required(VERSION 3.3)
project(vgmplayer)

if (NOT MSCV)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

find_package(SDL REQUIRED)
find_package(ZLIB REQUIRED)

file(GLOB_RECURSE H_FILES source/*.h)
file(GLOB_RECURSE C_FILES source/*.c)
file(GLOB_RECURSE CPP_FILES source/*.cpp)

include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(${SDL_INCLUDE_DIRS})

add_executable(vgmplayer ${CPP_FILES} ${C_FILES} ${H_FILES})
target_link_libraries(vgmplayer ${SDL_LIBRARY} ${ZLIB_LIBRARIES})
