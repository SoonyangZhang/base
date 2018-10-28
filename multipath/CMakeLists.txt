set(multipath_FILES
aggregaterate.cc
fakevideoconsumer.cc
fakevideogenerator.cc
mincostschedule.cc
mpsession.cc
mpsender.cc
mpreceiver.cc
path.cc
randomschedule.cc
)
add_library(multipath STATIC ${multipath_FILES})

set(EXECUTABLE_NAME "t_mp_sender")
add_executable(${EXECUTABLE_NAME} test_mp_sender.cc)
target_link_libraries(${EXECUTABLE_NAME} multipath razor rtc_base ns3log ns3file pthread)

set(EXECUTABLE_NAME "t_stream")
add_executable(${EXECUTABLE_NAME} test_stream.cc)
target_link_libraries(${EXECUTABLE_NAME} razor )

set(EXECUTABLE_NAME "t_seg")
add_executable(${EXECUTABLE_NAME} test_segment.cc)
target_link_libraries(${EXECUTABLE_NAME} multipath razor rtc_base ns3log pthread)

