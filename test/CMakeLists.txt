enable_testing()

# testing binary
add_executable(c_test_1 test_1.c)
target_link_libraries(c_test_1 linenoise)

add_test (NAME c_test_1 COMMAND $<TARGET_FILE:c_test_1>)