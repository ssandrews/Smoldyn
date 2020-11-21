include_directories(${CMAKE_SOURCE_DIR}/../source/Smoldyn)
add_subdirectory(S97_libsmoldyn)

add_custom_target(examples)
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
set(NOT_PY_EXAMPLES_NAME_WE regression lines)
foreach(_example ${PY_FILES})
    get_filename_component(_ex_name ${_example} NAME_WE)
    # these are not example script.
    if("${_ex_name}" IN_LIST NOT_PY_EXAMPLES_NAME_WE)
        continue()
    endif()
    message(STATUS "Found python example script: ${_example}.")
    get_filename_component(_ex_dir ${_example} DIRECTORY)
    add_custom_target(py${_ex_name} COMMAND
        PYTHONPATH=${CMAKE_BINARY_DIR}/py 
        MPLBACKEND=agg SMOLDYN_NO_PROMPT=1
        ${PYTHON_EXECUTABLE} ${_example}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Executing ${_example} (user prompts are disabled)" 
        VERBATIM)
    add_dependencies(pyexamples py${_ex_name})
endforeach()
