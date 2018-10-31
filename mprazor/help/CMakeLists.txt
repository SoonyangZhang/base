set(help_FILES
rate_record.cc
)
add_library(help STATIC ${help_FILES})
set(EXECUTABLE_NAME "t_record")
add_executable(${EXECUTABLE_NAME} test_record.cc)
target_link_libraries(${EXECUTABLE_NAME} help rtc_base pthread)

