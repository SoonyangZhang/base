include_directories(${CMAKE_SOURCE_DIR}/ns3log)
include_directories(${CMAKE_SOURCE_DIR}/ns3file)
set(ns3file_FILES
rng-stream.cc
rng-seed-manager.cc
random-variable-stream.cc
)
add_library(ns3file STATIC ${ns3file_FILES})

