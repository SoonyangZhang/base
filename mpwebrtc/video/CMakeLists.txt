include_directories(${CMAKE_SOURCE_DIR}/rtc_base)
set(video_FILES
filecamera.cc
videoframe.cc
nullsink.cc
videocodec.cc
imagerecord.cc
)
add_library(video STATIC ${video_FILES})
target_link_libraries(video rtc_base)

