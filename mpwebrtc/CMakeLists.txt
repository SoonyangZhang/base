PROJECT(project)
cmake_minimum_required(VERSION 2.6)
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#add_definitions(-fno-rtti)
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
	add_definitions(-DWEBRTC_WIN)
	add_definitions(-DNOMINMAX -DUNICODE)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
	add_definitions(-DWEBRTC_POSIX -DWEBRTC_LINUX)
endif()
include_directories(/home/zsy/webrtc-ns3/webrtc/)
LINK_DIRECTORIES(/home/zsy/webrtc-ns3/webrtc/build/rtc_base)
LINK_DIRECTORIES(/home/zsy/webrtc-ns3/webrtc/build/modules/utility)
LINK_DIRECTORIES(/home/zsy/webrtc-ns3/webrtc/build/system_wrappers)
LINK_DIRECTORIES(/home/zsy/webrtc-ns3/webrtc/build/api)
LINK_DIRECTORIES(/home/zsy/webrtc-ns3/webrtc/build/logging)
LINK_DIRECTORIES(/home/zsy/webrtc-ns3/webrtc/build/modules/pacing)
LINK_DIRECTORIES(/home/zsy/webrtc-ns3/webrtc/build/modules/congestion_controller)
LINK_DIRECTORIES(/home/zsy/webrtc-ns3/webrtc/build/modules/bitrate_controller)
LINK_DIRECTORIES(/home/zsy/webrtc-ns3/webrtc/build/modules/remote_bitrate_estimator)
LINK_DIRECTORIES(/home/zsy/webrtc-ns3/webrtc/build/modules/rtp_rtcp)
include_directories(${CMAKE_SOURCE_DIR}/ns3log)
include_directories(${CMAKE_SOURCE_DIR}/video)
include_directories(${CMAKE_SOURCE_DIR}/endecoder)
include_directories(${CMAKE_SOURCE_DIR}/multipath)
include_directories(${CMAKE_SOURCE_DIR}/ns3file)
include_directories(${CMAKE_SOURCE_DIR}/taskqueue/)
include_directories(${CMAKE_SOURCE_DIR}/razor/)

add_definitions(-std=c++11)
add_definitions(-DNS3_LOG_ENABLE)
add_subdirectory(ns3log)
add_subdirectory(video)
add_subdirectory(endecoder)
#add_subdirectory(multipath)
add_subdirectory(ns3file)
add_subdirectory(taskqueue)
add_subdirectory(test)
add_subdirectory(razor)
add_subdirectory(multipath)

set(EXECUTABLE_NAME "t_sink")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/video/nullsink_test.cc)
target_link_libraries(${EXECUTABLE_NAME} video task_queue ns3log rtc_base pthread)

set(EXECUTABLE_NAME "t_log")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/video/log_test.cc)
target_link_libraries(${EXECUTABLE_NAME}  ns3log)

include_directories(/usr/local/include)
LINK_DIRECTORIES(/usr/local/lib)
set(EXECUTABLE_NAME "t_encoder")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/video/encoder_test.cc)
target_link_libraries(${EXECUTABLE_NAME} video task_queue h264endecoder  ns3log rtc_base x264 avcodec avutil swscale pthread dl m)

include_directories(/usr/local/include)
LINK_DIRECTORIES(/usr/local/lib)
set(EXECUTABLE_NAME "t_endecoder")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/video/endecoder_test.cc)
target_link_libraries(${EXECUTABLE_NAME} video task_queue h264endecoder  ns3log rtc_base x264 avcodec avutil swscale pthread dl m)



set(EXECUTABLE_NAME "t_thread")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/video/main_test.cc)
target_link_libraries(${EXECUTABLE_NAME} task_queue rtc_base pthread)

set(EXECUTABLE_NAME "t_ffmpeg")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/endecoder/ffmpeg_test.cc)
target_link_libraries(${EXECUTABLE_NAME} video h264endecoder  ns3log rtc_base x264 avcodec avutil swscale pthread dl m)

#system_wrappers rtc_base api  logging  utility pacing bitrate_controller congestion_controller remote_bitrate_estimator

set(EXECUTABLE_NAME "psnr")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/tools/psnr.c)
target_link_libraries(${EXECUTABLE_NAME} m)




