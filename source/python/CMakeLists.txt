# python bindings


pybind11_add_module(pysmoldyn smoldyn.cpp)

target_compile_options(pysmoldyn PUBLIC -DSMOLDYN_VERSION=${SMOLDYN_VERSION})
target_link_libraries(pysmoldyn PRIVATE smoldyn_shared)

set_target_properties(pysmoldyn PROPERTIES 
    OUTPUT_NAME "smoldyn"
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_dependencies(pysmoldyn smoldyn_shared)

add_custom_command(TARGET pysmoldyn POST_BUILD
    COMMAND ${PYTHON_EXECUTABLE} setup.py bdist -v --bdist-base ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating bdist"
    VERBATIM)

enable_testing()
file(GLOB TEST_SCRIPTS "${CMAKE_SOURCE_DIR}/tests/test_*.py")
message(STATUS "${TEST_SCRIPTS} xxx")
foreach(_pyscript ${TEST_SCRIPTS})
    message(STATUS "Adding test ${_pyscript}") 
    get_filename_component(_test_name ${_pyscript} NAME_WE)
    get_filename_component(_test_dir ${_pyscript} DIRECTORY)
    add_test(NAME ${_test_name} COMMAND 
        ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/test_py.py
        WORKING_DIRECTORY ${_test_dir})
    set_tests_properties(${_test_name} PROPERTIES 
        ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}")
endforeach()
