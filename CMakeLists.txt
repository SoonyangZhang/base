PROJECT(project)
cmake_minimum_required(VERSION 2.6)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -O2")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#add_definitions(-fno-rtti)
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
	add_definitions(-DWEBRTC_WIN)
	add_definitions(-DNOMINMAX -DUNICODE)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
	add_definitions(-DWEBRTC_POSIX -DWEBRTC_LINUX)
endif()

include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/bbr)
include_directories(${CMAKE_SOURCE_DIR}/cc)
include_directories(${CMAKE_SOURCE_DIR}/estimator)
include_directories(${CMAKE_SOURCE_DIR}/pacing)
include_directories(${CMAKE_SOURCE_DIR}/sim_transport)

include_directories(${CMAKE_SOURCE_DIR}/rtc_base)
include_directories(${CMAKE_SOURCE_DIR}/ns3log)
include_directories(${CMAKE_SOURCE_DIR}/help)
include_directories(${CMAKE_SOURCE_DIR}/video)
include_directories(${CMAKE_SOURCE_DIR}/encoder)
set(razor_FILES
${CMAKE_SOURCE_DIR}/bbr/bbr_bandwidth_sample.c  
${CMAKE_SOURCE_DIR}/bbr/bbr_loss_rate_filter.c  
${CMAKE_SOURCE_DIR}/bbr/bbr_sender.c
${CMAKE_SOURCE_DIR}/bbr/bbr_common.c            
${CMAKE_SOURCE_DIR}/bbr/bbr_pacer.c             
${CMAKE_SOURCE_DIR}/bbr/windowed_filter.c
${CMAKE_SOURCE_DIR}/bbr/bbr_controller.c       
${CMAKE_SOURCE_DIR}/bbr/bbr_receiver.c
${CMAKE_SOURCE_DIR}/bbr/bbr_feedback_adpater.c  
${CMAKE_SOURCE_DIR}/bbr/bbr_rtt_stats.c

${CMAKE_SOURCE_DIR}/cc/razor_api.c  
${CMAKE_SOURCE_DIR}/cc/receiver_congestion_controller.c
${CMAKE_SOURCE_DIR}/cc/razor_log.c  
${CMAKE_SOURCE_DIR}/cc/sender_congestion_controller.c

${CMAKE_SOURCE_DIR}/common/cf_crc32.c  
${CMAKE_SOURCE_DIR}/common/cf_hex.c  
${CMAKE_SOURCE_DIR}/common/cf_list.c  
${CMAKE_SOURCE_DIR}/common/cf_skiplist.c  
${CMAKE_SOURCE_DIR}/common/cf_stream.c  
${CMAKE_SOURCE_DIR}/common/cf_unwrapper.c

${CMAKE_SOURCE_DIR}/estimator/ack_bitrate_estimator.c  
${CMAKE_SOURCE_DIR}/estimator/kalman_filter.c
${CMAKE_SOURCE_DIR}/estimator/aimd_rate_control.c      
${CMAKE_SOURCE_DIR}/estimator/overuse_detector.c
${CMAKE_SOURCE_DIR}/estimator/bitrate_controller.c     
${CMAKE_SOURCE_DIR}/estimator/rate_stat.c
${CMAKE_SOURCE_DIR}/estimator/cc_loss_stat.c           
${CMAKE_SOURCE_DIR}/estimator/remote_bitrate_estimator.c
${CMAKE_SOURCE_DIR}/estimator/delay_base_bwe.c         
${CMAKE_SOURCE_DIR}/estimator/remote_estimator_proxy.c
${CMAKE_SOURCE_DIR}/estimator/estimator_common.c       
${CMAKE_SOURCE_DIR}/estimator/sender_bandwidth_estimator.c
${CMAKE_SOURCE_DIR}/estimator/feedback_adapter.c       
${CMAKE_SOURCE_DIR}/estimator/sender_history.c
${CMAKE_SOURCE_DIR}/estimator/inter_arrival.c         
${CMAKE_SOURCE_DIR}/estimator/trendline.c

${CMAKE_SOURCE_DIR}/pacing/alr_detector.c  
${CMAKE_SOURCE_DIR}/pacing/interval_budget.c  
${CMAKE_SOURCE_DIR}/pacing/pacer_queue.c  
${CMAKE_SOURCE_DIR}/pacing/pace_sender.c

${CMAKE_SOURCE_DIR}/sim_transport/sim_external.c  
${CMAKE_SOURCE_DIR}/sim_transport/sim_proto.c     
${CMAKE_SOURCE_DIR}/sim_transport/sim_sender.c
${CMAKE_SOURCE_DIR}/sim_transport/sim_limiter.c   
${CMAKE_SOURCE_DIR}/sim_transport/sim_receiver.c  
${CMAKE_SOURCE_DIR}/sim_transport/sim_session.c
)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
set(razor_FILES ${razor_FILES}
${CMAKE_SOURCE_DIR}/common/platform/windows/mscc.c
)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
set(razor_FILES ${razor_FILES}
${CMAKE_SOURCE_DIR}/common/platform/posix/posix.c
)
endif()

add_library(razor STATIC ${razor_FILES})
add_definitions(-std=c++11)
add_definitions(-DNS3_LOG_ENABLE)
add_subdirectory(rtc_base)
add_subdirectory(ns3log)
add_subdirectory(help)
add_subdirectory(video)
add_subdirectory(encoder)
set(EXECUTABLE_NAME "sim_sender")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/sim_test/sim_sender/sim_sender_test.cpp)
target_link_libraries(${EXECUTABLE_NAME}  help razor rtc_base pthread)

set(EXECUTABLE_NAME "sim_receiver")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/sim_test/sim_receiver/sim_receiver_test.cpp)
target_link_libraries(${EXECUTABLE_NAME}  razor pthread)


set(EXECUTABLE_NAME "get_ip")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/tool/get_ip.cc)

set(EXECUTABLE_NAME "t_sink")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/video/nullsink_test.cc)
target_link_libraries(${EXECUTABLE_NAME} video ns3log rtc_base pthread)

set(EXECUTABLE_NAME "t_log")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/video/log_test.cc)
target_link_libraries(${EXECUTABLE_NAME}  ns3log)

include_directories(/usr/local/include)
LINK_DIRECTORIES(/usr/local/lib)
set(EXECUTABLE_NAME "t_encoder")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/video/encoder_test.cc)
target_link_libraries(${EXECUTABLE_NAME} video echo_h264  ns3log rtc_base x264 avcodec avutil swscale pthread dl m)

set(EXECUTABLE_NAME "t_thread")
add_executable(${EXECUTABLE_NAME} ${CMAKE_SOURCE_DIR}/video/main_test.cc)
target_link_libraries(${EXECUTABLE_NAME}  rtc_base)


