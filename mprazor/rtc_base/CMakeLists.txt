add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-std=c++11)
#-fno-rtti
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_subdirectory(rtc_base)
include_directories(.)

