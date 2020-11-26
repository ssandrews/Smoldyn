add_executable(test1 test1.c)
target_link_libraries(test1 PUBLIC smoldyn_static)

add_executable(testcode ./testcode/testcode.c)
target_link_libraries(testcode PUBLIC smoldyn_static)

enable_testing()
add_test(NAME test_libsmodyn1 COMMAND $<TARGET_FILE:test1>)
add_test(NAME test_libsmodyn2 COMMAND $<TARGET_FILE:testcode>)
