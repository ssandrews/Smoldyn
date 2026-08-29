# Uses Catch2 v3, pulled via CMake FetchContent.
#
# These tests exercise the pure C library routines in source/libSteve directly,
# and a few self-contained parsing helpers pulled from the Smoldyn core via the
# already-built smoldyn_static library.

include(FetchContent)

set(CATCH2_VERSION "v3.5.2")
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        ${CATCH2_VERSION}
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

# Where the libSteve sources live. We compile only the specific production
# translation units each test needs, avoiding OpenGL/TIFF dependencies.
set(LIBSTEVE_DIR ${CMAKE_SOURCE_DIR}/source/libSteve)

# Helper to turn a test target into a registered CTest test.
function(add_smoldyn_cpp_test target)
    target_link_libraries(${target} PRIVATE Catch2::Catch2WithMain)
    target_include_directories(${target} PRIVATE ${LIBSTEVE_DIR})
    add_test(NAME ${target} COMMAND ${target})
endfunction()

#
# test_math2: special functions, combinatorics, interval + regression routines.
#
add_executable(test_math2 test_math2.cpp
    ${LIBSTEVE_DIR}/math2.c)
add_smoldyn_cpp_test(test_math2)

#
# test_string2: number detection, word splitting, string->number reads,
# math-expression evaluation.
#
add_executable(test_string2 test_string2.cpp
    ${LIBSTEVE_DIR}/string2.c
    ${LIBSTEVE_DIR}/math2.c
    ${LIBSTEVE_DIR}/SFMT/SFMT.c)
add_smoldyn_cpp_test(test_string2)

#
# test_reaction_kernels: reaction-rate / surface-parameter computation kernels
# and random-sampling invariants.
#
add_executable(test_reaction_kernels test_reaction_kernels.cpp
    ${LIBSTEVE_DIR}/rxn2Dparam.c
    ${LIBSTEVE_DIR}/SurfaceParam.c
    ${LIBSTEVE_DIR}/random2.c
    ${LIBSTEVE_DIR}/math2.c
    ${LIBSTEVE_DIR}/SFMT/SFMT.c)
add_smoldyn_cpp_test(test_reaction_kernels)

#
# test_known_bugs: intentionally-failing tests that assert the correct behaviour
# for known libSteve defects. These are expected to fail until the underlying
# functions are fixed; do not remove them once the bugs are resolved.
#
add_executable(test_known_bugs test_known_bugs.cpp)
target_link_libraries(test_known_bugs PRIVATE Catch2::Catch2WithMain smoldyn_static)
target_include_directories(test_known_bugs PRIVATE ${LIBSTEVE_DIR})
add_test(NAME test_known_bugs COMMAND test_known_bugs)

#
# test_geometry: distances, areas, volumes and point-in tests.
#
add_executable(test_geometry_cpp test_geometry.cpp
    ${LIBSTEVE_DIR}/Geometry.c
    ${LIBSTEVE_DIR}/math2.c)
add_smoldyn_cpp_test(test_geometry_cpp)

#
# test_core_parsing: self-contained Smoldyn core parsers (enum <-> string).
# Linked against the real smoldyn_static so these use production code. The
# target's PUBLIC DEP_LIBS (OpenGL/TIFF/etc.) propagate automatically.
#
add_executable(test_core_parsing test_core_parsing.cpp)
target_link_libraries(test_core_parsing PRIVATE Catch2::Catch2WithMain smoldyn_static)
add_test(NAME test_core_parsing COMMAND test_core_parsing)
