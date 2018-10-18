include_directories(${CMAKE_SOURCE_DIR}/encoder)
include_directories(/usr/local/include)
LINK_DIRECTORIES(/usr/local/lib)
set(encoder_FILES
echo_h264_encoder.cc
)
add_library(echo_h264 STATIC ${encoder_FILES})
