include_directories(${CMAKE_SOURCE_DIR}/../source/Smoldyn)
add_subdirectory(S97_libsmoldyn)

set(EXAMPLE_SCRIPTS S10_boxes/box.txt S3_space/bounds2.txt)

add_custom_target(examples)
foreach(_example ${EXAMPLE_SCRIPTS})
    message(STATUS "Adding ${_example} to runnable examples")
    get_filename_component(_ex_name ${_example} NAME_WE)
    get_filename_component(_ex_dir ${_example} DIRECTORY)
    add_custom_target(${_ex_name} COMMAND
        ${PYTHON_EXECUTABLE} -m smoldyn ${_example}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Executing ${_example}" VERBATIM)
    set_target_properties(${_ex_name} PROPERTIES 
        ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/py")
    add_dependencies(examples ${_ex_name})
endforeach()
