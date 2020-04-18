# python bindings
find_package(pybind11 REQUIRED)

pybind11_add_module(_smoldyn pysmoldyn.cpp)

target_compile_options(_smoldyn PUBLIC -DSMOLDYN_VERSION=${SMOLDYN_VERSION})
target_link_libraries(_smoldyn PRIVATE smoldyn_shared)

set_target_properties(_smoldyn PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_dependencies(_smoldyn smoldyn_shared)

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
        ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}")
endforeach()
