include_directories(${CMAKE_SOURCE_DIR}/lib)
set(encoder_FILES
echo_h264_encoder.cc
)
add_library(echo_h264 STATIC ${encoder_FILES})