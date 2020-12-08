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
        COMMAND 
            ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_BINARY_DIR}/py MPLBACKEND=agg SMOLDYN_NO_PROMPT=1 
            ${Python3_EXECUTABLE} ${PYSCRIPT_RUN_WITH_TIMEOUT} ${_example}
        COMMENT "Executing ${_example} (user prompts are disabled)" 
        VERBATIM)

    add_dependencies(pyexamples py${_ex_name})

    # add to ctest as well.
    add_test(NAME test_${_ex_name} 
        COMMAND ${Python3_EXECUTABLE} ${PYSCRIPT_RUN_WITH_TIMEOUT} ${_example})

    set_tests_properties(test_${_ex_name} PROPERTIES 
            ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/py;MPLBACKEND=agg;SMOLDYN_NO_PROMPT=1")

endforeach()

include_directories(${CMAKE_SOURCE_DIR}/source/Smoldyn)
include_directories(${CMAKE_BINARY_DIR})

add_executable(test1 S97_libsmoldyn/test1.cpp)
target_link_libraries(test1 smoldyn_static)

add_executable(testcode S97_libsmoldyn/testcode/testcode.cpp)
target_link_libraries(testcode smoldyn_static)

if(UNIX)
   # can't get them work on Windows.
   add_test(NAME test_test1.cpp COMMAND $<TARGET_FILE:test1>)
   add_test(NAME test_testcode.cpp COMMAND $<TARGET_FILE:testcode>)
endif()
