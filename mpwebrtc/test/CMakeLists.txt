set(EXECUTABLE_NAME "t_module")
add_executable(${EXECUTABLE_NAME} test_module.cc)
target_link_libraries(${EXECUTABLE_NAME}  utility system_wrappers rtc_base  pthread)



