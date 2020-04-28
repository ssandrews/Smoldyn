# python bindings

pybind11_add_module(_smoldyn Simulation.cpp SmoldynSpecies.cpp module.cpp)

target_compile_options(_smoldyn PUBLIC -DSMOLDYN_VERSION=${SMOLDYN_VERSION})
target_include_directories(_smoldyn PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../pybind11/include)

set_target_properties(_smoldyn PROPERTIES 
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/smoldyn)

add_dependencies(_smoldyn smoldyn_static)

if(APPLE)
    target_link_libraries(_smoldyn PUBLIC -Wl,-all_load smoldyn_static -Wl,-noall_load)
else(APPLE)
    target_link_libraries(_smoldyn PUBLIC 
        -Wl,--whole-archive 
        smoldyn_static
        -Wl,--no-whole-archive) 
endif()

add_custom_command(TARGET _smoldyn POST_BUILD
    COMMAND ${PYTHON_EXECUTABLE} setup.py bdist_wheel 
        -d ${CMAKE_BINARY_DIR}/wheel
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating wheel"
    VERBATIM)

add_custom_target(pyinstall
    DEPENDS _smoldyn
    COMMAND ${PYTHON_EXECUTABLE} -m pip install wheel/*
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "installing wheel")
    #VERBATIM)
    

add_custom_target(pyinstall_venv
    DEPENDS _smoldyn
    COMMAND ${PYTHON_EXECUTABLE} setup.py install
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "RUNNING: python3 setup.py install"
    VERBATIM)

add_custom_target(pyuninstall
    COMMAND ${PYTHON_EXECUTABLE} -m pip uninstall smoldyn
    COMMENT "RUNNING: `pip uninstall smoldyn`"
    VERBATIM)

add_custom_target(pydevel
    DEPENDS _smoldyn
    COMMAND ${PYTHON_EXECUTABLE} setup.py develop --user
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "RUNNING: python3 setup.py develop --user"
    VERBATIM)


enable_testing()

###  python tests ###
file(GLOB TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.py")
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

# Run tests using pytest.
add_custom_target(pytest 
    DEPENDS pydevel
    COMMAND ${PYTHON_EXECUTABLE} -m pytest ${CMAKE_SOURCE_DIR}
    COMMENT "Running pytest."
    VERBATIM)
