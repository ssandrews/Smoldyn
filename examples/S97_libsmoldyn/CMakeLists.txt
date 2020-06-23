add_executable(test1 test1.cpp)
target_link_libraries(test1 smoldyn_shared)

add_executable(testcode ./testcode/testcode.cpp)
target_link_libraries(testcode smoldyn_shared)

enable_testing()
add_test(NAME test_libsmodyn1 COMMAND $<TARGET_FILE:test1>)
add_test(NAME test_libsmodyn2 COMMAND $<TARGET_FILE:testcode>)
