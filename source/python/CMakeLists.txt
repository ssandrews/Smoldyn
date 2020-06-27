# python binding.

if("${CMAKE_VERSION}" VERSION_LESS "3.12")
    if(NOT PYTHON_EXECUTABLE)
        find_package(PythonInterp REQUIRED)
    endif()
    find_package(PythonLibs REQUIRED)
else()
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
    set(PYTHON_INCLUDE_DIRS ${Python3_INCLUDE_DIRS})
endif()

set(PY_SMOLDYN_OUTDIR ${CMAKE_BINARY_DIR}/py)
file(MAKE_DIRECTORY ${PY_SMOLDYN_OUTDIR})

pybind11_add_module(_smoldyn MODULE Smoldyn.cpp module.cpp)

target_compile_options(_smoldyn PRIVATE -DSMOLDYN_VERSION=${SMOLDYN_VERSION})
target_include_directories(_smoldyn PRIVATE 
    ${PYTHON_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../pybind11/include)

set_target_properties(_smoldyn PROPERTIES 
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY ${PY_SMOLDYN_OUTDIR}/smoldyn/)

add_custom_target(copy_python_tree 
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/source/python/smoldyn ${PY_SMOLDYN_OUTDIR}/smoldyn
  COMMENT "Copying python source tree to binary directory"
  VERBATIM)

add_dependencies(_smoldyn smoldyn_static copy_python_tree)

target_link_libraries(_smoldyn PRIVATE smoldyn_static)


# copy setup.py.in to setup.py
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in
    ${PY_SMOLDYN_OUTDIR}/setup.py)

add_custom_target(wheel DEPENDS _smoldyn 
    COMMAND ${PYTHON_EXECUTABLE} setup.py bdist_wheel -d ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${PY_SMOLDYN_OUTDIR}
    COMMENT "Generating wheel in ${CMAKE_BINARY_DIR}"
    VERBATIM)

  add_custom_target(pyinstall
    COMMAND ${PYTHON_EXECUTABLE} setup.py install 
      --prefix ${CMAKE_INSTALL_PREFIX}
    WORKING_DIRECTORY ${PY_SMOLDYN_OUTDIR}
    COMMENT "RUNNING: python3 setup.py install --prefix ${CMAKE_INSTALL_PREFIX}"
    VERBATIM)

add_custom_target(pyinstall_venv
    DEPENDS _smoldyn
    COMMAND ${PYTHON_EXECUTABLE} setup.py install
    WORKING_DIRECTORY ${PY_SMOLDYN_OUTDIR}
    COMMENT "RUNNING: python3 setup.py install"
    VERBATIM)

add_custom_target(pyuninstall
    COMMAND ${PYTHON_EXECUTABLE} -m pip uninstall smoldyn
    COMMENT "RUNNING: `pip uninstall smoldyn`"
    VERBATIM)

add_custom_target(pydevel
    DEPENDS _smoldyn
    COMMAND ${PYTHON_EXECUTABLE} setup.py develop --user
    WORKING_DIRECTORY ${PY_SMOLDYN_OUTDIR}
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
      ENVIRONMENT "PYTHONPATH=${PY_SMOLDYN_OUTDIR}")
endforeach()

# Run tests using pytest.
add_custom_target(pytest 
    DEPENDS pydevel
    COMMAND ${PYTHON_EXECUTABLE} -m pip install pytest || echo "Failed to install pytest"
    COMMAND ${PYTHON_EXECUTABLE} -m pytest ${CMAKE_SOURCE_DIR}
    COMMENT "Running pytest."
    VERBATIM)
