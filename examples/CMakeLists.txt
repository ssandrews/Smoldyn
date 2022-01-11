enable_testing()
add_custom_target(examples)

# generate script to run python script.
configure_file(${CMAKE_SOURCE_DIR}/scripts/run_pyscript.py.in ${CMAKE_BINARY_DIR}/run_pyscript.py)
set(PYSCRIPT_RUN_WITH_TIMEOUT ${CMAKE_BINARY_DIR}/run_pyscript.py)

# Add all python scripts to pyexample target.
add_custom_target(pyexamples)
file(GLOB_RECURSE PY_FILES "${CMAKE_CURRENT_SOURCE_DIR}/**.py")

# ignore these scripts; they are not smoldyn script.
set(NOT_PY_EXAMPLES_NAME_WE regression lines pyscript)

foreach(_example ${PY_FILES})
    get_filename_component(_ex_name ${_example} NAME_WE)

    # these are not example script.
    if("${_ex_name}" IN_LIST NOT_PY_EXAMPLES_NAME_WE)
        continue()
    endif()

    # message(STATUS "Found python example script: ${_example}.")
    get_filename_component(_ex_dir ${_example} DIRECTORY)

    # run the above script in modified environment (portable). Use
    # a wrapper script to timeout at 15 sec.
    add_custom_target(py${_ex_name}
        COMMAND ${CMAKE_COMMAND} -E env
                PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/../source/python
                MPLBACKEND=agg
                SMOLDYN_NO_PROMPT=1
            ${Python3_EXECUTABLE} ${PYSCRIPT_RUN_WITH_TIMEOUT} ${_example}
        COMMENT "Executing ${_example} (user prompts are disabled)"
        WORKING_DIRECTORY ${_ex_dir}
        VERBATIM)

    add_dependencies(pyexamples py${_ex_name})

    # add to ctest as well.
    add_test(NAME example_${_ex_name}
        COMMAND ${Python3_EXECUTABLE} ${PYSCRIPT_RUN_WITH_TIMEOUT} ${_example})

    # MPLBACKEND=agg : makes sure that X11 is not needed while testing on CI.
    # SMOLDYN_NO_PROMPT=1 : makes sure that shiftQ is not needed to be pressed by
    # anyone at the end of simulation.
    set_tests_properties(example_${_ex_name} PROPERTIES
            ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/../source/python;MPLBACKEND=agg;SMOLDYN_NO_PROMPT=1")

endforeach()

include_directories(${CMAKE_SOURCE_DIR}/source/Smoldyn)
include_directories(${CMAKE_BINARY_DIR})

add_executable(test1c S97_libsmoldyn/test1.c)
target_link_libraries(test1c PRIVATE smoldyn_static)

add_executable(test1 S97_libsmoldyn/test1.cpp)
target_link_libraries(test1 PRIVATE smoldyn_static)

add_executable(testcode S97_libsmoldyn/testcode/testcode.cpp)
target_link_libraries(testcode PRIVATE smoldyn_static)

if(UNIX)
   # can't get them to work on Windows.
   add_test(NAME test_test1.cpp COMMAND $<TARGET_FILE:test1>)
   add_test(NAME test_test1.c COMMAND $<TARGET_FILE:test1c>)
   add_test(NAME test_testcode.cpp COMMAND $<TARGET_FILE:testcode>)
   set_tests_properties(test_test1.cpp test_test1.c test_testcode.cpp PROPERTIES
       ENVIRONMENT SMOLDYN_NO_PROMPT=1)
endif()
