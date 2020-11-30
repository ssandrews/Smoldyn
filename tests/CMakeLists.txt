enable_testing()

add_test(test_comment_var 
    ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/connect_test_var.py)

add_test(test_comment_func
    ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/connect_test_func.py)

set_tests_properties(test_comment_var test_comment_func 
    PROPERTIES ENVIRONMENT "PYTHONPATH=${PY_SMOLDYN_OUTDIR}")
