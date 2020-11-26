add_custom_target(examples)

# generate script to run python script.
configure_file(${CMAKE_SOURCE_DIR}/scripts/run_pyscript.sh.in ${CMAKE_BINARY_DIR}/run_pyscript.sh)
set(PYSCRIPT_RUN_SCRIPT ${CMAKE_BINARY_DIR}/run_pyscript.sh)

# Script examples. Only few are added.
set(EXAMPLE_SCRIPTS S10_boxes/box.txt S3_space/bounds2.txt)

foreach(_example ${EXAMPLE_SCRIPTS})
    # message(STATUS "Adding ${_example}")
    get_filename_component(_ex_name ${_example} NAME_WE)
    get_filename_component(_ex_dir ${_example} DIRECTORY)
    add_custom_target(${_ex_name} COMMAND
        PYTHONPATH=${CMAKE_BINARY_DIR}/py ${PYTHON_EXECUTABLE} -m smoldyn ${_example}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Executing ${_example}" VERBATIM)
    add_dependencies(examples ${_ex_name})
endforeach()

# Python scripts.
add_custom_target(pyexamples)
file(GLOB_RECURSE PY_FILES "${CMAKE_CURRENT_SOURCE_DIR}/**.py")
set(NOT_PY_EXAMPLES_NAME_WE regression lines pyscript)
foreach(_example ${PY_FILES})
    get_filename_component(_ex_name ${_example} NAME_WE)
    # these are not example script.
    if("${_ex_name}" IN_LIST NOT_PY_EXAMPLES_NAME_WE)
        continue()
    endif()
    # message(STATUS "Found python example script: ${_example}.")
    get_filename_component(_ex_dir ${_example} DIRECTORY)
    add_custom_target(py${_ex_name} COMMAND
        PYTHONPATH=${CMAKE_BINARY_DIR}/py 
        MPLBACKEND=agg SMOLDYN_NO_PROMPT=1
        ${PYSCRIPT_RUN_SCRIPT} ${_example}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Executing ${_example} (user prompts are disabled)" 
        VERBATIM)
    add_dependencies(pyexamples py${_ex_name})
endforeach()

include_directories(${CMAKE_SOURCE_DIR}/source/Smoldyn)
include_directories(${CMAKE_BINARY_DIR})

add_executable(test1 S97_libsmoldyn/test1.cpp)
target_link_libraries(test1 smoldyn_static)

add_executable(testcode S97_libsmoldyn/testcode/testcode.cpp)
target_link_libraries(testcode smoldyn_static)

enable_testing()
add_test(NAME test_test1.cpp COMMAND $<TARGET_FILE:test1>)
add_test(NAME test_testcode.cpp COMMAND $<TARGET_FILE:testcode>)
