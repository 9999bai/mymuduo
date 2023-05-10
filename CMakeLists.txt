cmake_minimum_required(VERSION 3.0.0)

set(projectName mymuduo)
project(${projectName})

include_directories(.)
aux_source_directory(. DIR_SRCS)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -pthread -fPIC")
add_library(${projectName} SHARED ${DIR_SRCS})

