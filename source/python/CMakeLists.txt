# python binding.
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
set(PYTHON_INCLUDE_DIRS ${Python3_INCLUDE_DIRS})

set(PY_SMOLDYN_OUTDIR ${CMAKE_BINARY_DIR}/py)
file(MAKE_DIRECTORY ${PY_SMOLDYN_OUTDIR})

# recompile libsmoldyn for PYTHON.
set_source_files_properties(${SRC_FILES} PROPERTIES LANGUAGE CXX )
add_library(_pysmoldyn STATIC ${SRC_FILES})
target_compile_options(_pysmoldyn PRIVATE -DENABLE_PYTHON_CALLBACK)
target_include_directories(_pysmoldyn PRIVATE ${PYTHON_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/source/external/pybind11/include)

set_target_properties(_pysmoldyn PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(_pysmoldyn PRIVATE ${DEP_LIBS})
target_link_libraries(_pysmoldyn PUBLIC ${PYTHON_LIBRARIES})

pybind11_add_module(_smoldyn MODULE CallbackFunc.cpp Smoldyn.cpp module.cpp)

target_compile_options(_smoldyn PRIVATE -DSMOLDYN_VERSION=${SMOLDYN_VERSION} 
    -DENABLE_PYTHON_CALLBACK)
target_include_directories(_smoldyn PRIVATE 
    ${PYTHON_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/source/external/pybind11/include)

set_target_properties(_smoldyn PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${PY_SMOLDYN_OUTDIR}/smoldyn/)

add_custom_target(copy_python_tree 
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/source/python/smoldyn ${PY_SMOLDYN_OUTDIR}/smoldyn
  COMMENT "Copying python source tree to binary directory"
  VERBATIM)

add_dependencies(_smoldyn _pysmoldyn copy_python_tree)
target_link_libraries(_smoldyn PRIVATE _pysmoldyn)


# copy setup.py.in to setup.py
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in
    ${PY_SMOLDYN_OUTDIR}/setup.py)

set(WHEEL_PYTHON_TAG py${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR})
set(WHEEL_ABI_TAG cp${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR})
set(WHEEL_PLATFORM_TAG cp${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR})
add_custom_target(wheel DEPENDS _smoldyn 
    COMMAND ${PYTHON_EXECUTABLE} setup.py bdist_wheel 
	--python-tag ${WHEEL_PYTHON_TAG}
        -d ${CMAKE_BINARY_DIR}
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

add_custom_target(doc_api_html
    DEPENDS _smoldyn
    COMMAND sphinx-build ${CMAKE_CURRENT_SOURCE_DIR}/docs/source
        ${PY_SMOLDYN_OUTDIR}/html
    WORKING_DIRECTORY ${PY_SMOLDYN_OUTDIR}
    COMMENT "Generating Python's API doc (HTML)"
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

# Collect python examples.
add_custom_target(pyexamples)

file(GLOB_RECURSE PY_MODELS "../../examples/*.py")
foreach(_py_model ${PY_MODELS})
    #message(STATUS "Adding ${_py_model} to runnable examples")
    get_filename_component(_ex_name ${_py_model} NAME_WE)
    get_filename_component(_ex_dir ${_py_model} DIRECTORY)
    add_custom_target(${_ex_name} 
        COMMAND PYTHONPATH=${PY_SMOLDYN_OUTDIR} 
            MPLBACKEND=Agg
            SMOLDYN_NON_INTERACTIVE=1
            ${PYTHON_EXECUTABLE} ${_py_model}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Executing ${_py_model}" VERBATIM)
    add_dependencies(pyexamples ${_ex_name})
endforeach()


enable_testing()
add_test(NAME pytest 
    COMMAND ${PYTHON_EXECUTABLE} -m pytest -q ${CMAKE_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(pytest PROPERTIES ENVIRONMENT "PYTHONPATH=${PY_SMOLDYN_OUTDIR}")
