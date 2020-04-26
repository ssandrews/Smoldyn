add_executable(test1 test1.c)
target_link_libraries(test1 smoldyn_shared)

add_executable(testcode ./testcode/testcode.c)
target_link_libraries(testcode smoldyn_shared)

set(SRC_FILES test1.c testcode/testcode.c)
set_source_files_properties(${SRC_FILES} PROPERTIES LANGUAGE CXX )

