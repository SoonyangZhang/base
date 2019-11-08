PROJECT(project)
cmake_minimum_required(VERSION 2.6)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall  -O2")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -O2")
add_definitions(-D__STDC_CONSTANT_MACROS)
include_directories(/usr/local/include)

LINK_DIRECTORIES(/usr/local/lib)

include_directories(${CMAKE_SOURCE_DIR}/base)
include_directories(${CMAKE_SOURCE_DIR}/encoder)
include_directories(${CMAKE_SOURCE_DIR}/logging)
include_directories(${CMAKE_SOURCE_DIR}/tools)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)
set(base_FILES
base/random.cc
)
add_library(base STATIC ${base_FILES})

set(encoder_FILES
encoder/h264_decoder.cc
encoder/echo_h264_encoder.cc
encoder/bw_reader.cc
encoder/camera.cc
encoder/split.cc
)
add_library(h264encoder STATIC ${encoder_FILES})

set(psnr_LIB
tools/psnr.c
)
add_library(psnr STATIC ${psnr_LIB})

set(EXECUTABLE_NAME "encoder")
add_executable(${EXECUTABLE_NAME} x264_encoder_test.cc)
target_link_libraries(${EXECUTABLE_NAME}  h264encoder x264 avcodec avutil swscale pthread dl m)

set(EXECUTABLE_NAME "t_psnr")
add_executable(${EXECUTABLE_NAME} tools/t_psnr.cc)
target_link_libraries(${EXECUTABLE_NAME} psnr m)

set(EXECUTABLE_NAME "t_test")
add_executable(${EXECUTABLE_NAME} t_test.cc)
target_link_libraries(${EXECUTABLE_NAME} h264encoder psnr base x264 avcodec avutil swscale pthread dl m )
