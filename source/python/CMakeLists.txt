# python bindings


pybind11_add_module(_smoldyn smoldyn.cpp)

target_compile_options(_smoldyn PUBLIC -DSMOLDYN_VERSION=${SMOLDYN_VERSION})


set_target_properties(_smoldyn PROPERTIES 
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/smoldyn)

add_dependencies(_smoldyn smoldyn_shared)

target_link_libraries(_smoldyn PUBLIC 
    -Wl,--whole-archive 
    smoldyn_shared
    -Wl,--no-whole-archive)

add_custom_command(TARGET _smoldyn POST_BUILD
    COMMAND ${PYTHON_EXECUTABLE} setup.py bdist -v --bdist-base ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating bdist"
    VERBATIM)

add_custom_target(pyinstall
    DEPENDS pyuninstall _smoldyn
    COMMAND ${PYTHON_EXECUTABLE} setup.py develop --user
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "python3 setup.py develop --user"
    VERBATIM)

add_custom_target(pyuninstall
    COMMAND ${PYTHON_EXECUTABLE} -m pip uninstall pymoose
    COMMENT "Uninstall develop"
    VERBATIM)


enable_testing()

###  python tests ###
file(GLOB TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.py")
message(STATUS "${TEST_SCRIPTS} xxx")
foreach(_pyscript ${TEST_SCRIPTS})
    message(STATUS "Adding test ${_pyscript}") 
    get_filename_component(_test_name ${_pyscript} NAME_WE)
    get_filename_component(_test_dir ${_pyscript} DIRECTORY)
    add_test(NAME ${_test_name} COMMAND 
        ${PYTHON_EXECUTABLE} ${_pyscript}
        WORKING_DIRECTORY ${_test_dir})
    set_tests_properties(${_test_name} PROPERTIES 
        ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}")
endforeach()
