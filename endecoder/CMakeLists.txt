include_directories(${CMAKE_SOURCE_DIR}/encoder)
include_directories(/usr/local/include)
LINK_DIRECTORIES(/usr/local/lib)
set(endecoder_FILES
echo_h264_encoder.cc
#echo_h264_decoder.cc
H264Decoder.cc
)
add_library(h264endecoder STATIC ${endecoder_FILES})
