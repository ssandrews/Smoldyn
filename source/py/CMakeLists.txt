# python bindings
find_package(pybind11 REQUIRED)

pybind11_add_module(smoldyn MODULE pysmoldyn.cpp)

target_compile_options(smoldyn PUBLIC -DSMOLDYN_VERSION=${SMOLDYN_VERSION})

target_link_libraries(smoldyn PRIVATE smoldyn_shared)

set_target_properties(smoldyn PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_dependencies(smoldyn smoldyn_shared)

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
