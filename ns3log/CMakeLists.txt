include_directories(${CMAKE_SOURCE_DIR}/ns3log)
set(log_FILES
log.cc
fatal-impl.cc
)
add_library(ns3log STATIC ${log_FILES})

