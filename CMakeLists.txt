# Main CMakeLists.txt file to build Smoldyn, its libraries, and its utilities using CMake.
# Written by Steve Andrews, April 16, 2012. Updated by Steve Andrews since then.
# This file, and other files of the Smoldyn project, are licensed with LGPL license.
# The primary Smoldyn web site is http://www.smoldyn.org.
# Please report bugs and problems to support@smoldyn.org.

########## Basic setup ##########
cmake_minimum_required(VERSION 3.14)

project(smoldyn LANGUAGES CXX C)

# Custom scripts to find some modules such as VTK.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# CI/CD can also set version string. If not, set using git tags or current
# date.
if(NOT SMOLDYN_VERSION)
    find_package(Git QUIET)
    if(GIT_FOUND)
        # Is the current commit tagged? If yes, this command will return the
        # tag else it returns empty. When a tag is found, it is a release.
        execute_process(COMMAND ${GIT_EXECUTABLE} tag --points-at HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE SMOLDYN_TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(SMOLDYN_TAG)
            # drop v from the tag to get the version info.
            string(SUBSTRING ${SMOLDYN_TAG} 1 -1 SMOLDYN_VERSION)
            message(STATUS " -- Found a release tag ${SMOLDYN_TAG}")
        else()
            # nightly release.
            string(TIMESTAMP STAMP "%Y%m%d")
            set(SMOLDYN_VERSION "2.66.dev${STAMP}")
        endif()
    endif()
endif()
message(STATUS "Smoldyn version is set to: '${SMOLDYN_VERSION}'")


######### Targets to build ###########

option(OPTION_TARGET_SMOLDYN "Create stand-alone Smoldyn program" ON)
option(OPTION_TARGET_LIBSMOLDYN "Create LibSmoldyn library" ON)

message(STATUS "Compiling for Smoldyn: ${OPTION_TARGET_SMOLDYN}")
message(STATUS "Compiling for Libsmoldyn: ${OPTION_TARGET_LIBSMOLDYN}")


####### Compiling options ##########

option(OPTION_VCELL "Compile Smoldyn for VCell" OFF)
option(OPTION_MINGW "Cross-compile for Windows using MinGW compiler" OFF)

option(OPTION_NSV "Compile Smoldyn with NextSubvolume functionality" ON)
option(OPTION_VTK "Compile Smoldyn with VTK functionality" OFF)
option(OPTION_STATIC "Compile Smoldyn with static libraries" OFF)
option(OPTION_USE_OPENGL "Build with OpenGL support" ON)
option(OPTION_USE_LIBTIFF "Build with LibTiff support" ON)
option(OPTION_USE_ZLIB "Build with Zlib support" OFF)
option(OPTION_PYTHON  "Build Python module" ON)
option(OPTION_EXAMPLES "Run Libsmoldyn tests" OFF)
option(OPTION_DOCS "Generate documentation" OFF)


#
# Developer options. Enables Address Sanitizer to detects memory leak etc. Most
# memory leaks are in GL interface.
#
option(OPTION_STRICT_BUILD "Treat most warnings as error." OFF)
option(OPTION_ASAN "Enable Address Sanitizer" OFF)

if (OPTION_VCELL)
    set(OPTION_USE_LIBTIFF OFF)
    set(OPTION_USE_ZLIB OFF)
    set(HAVE_ZLIB TRUE)
endif ()

if (OPTION_PYTHON)
    if(NOT OPTION_TARGET_LIBSMOLDYN)
        set(OPTION_TARGET_LIBSMOLDYN ON)
        message(STATUS "Compiling for Libsmoldyn: ${OPTION_TARGET_LIBSMOLDYN}")
    endif ()
endif ()

message(STATUS "Option to compile Smoldyn for VCell: ${OPTION_VCELL}")
message(STATUS "Option to cross-compile for Windows using MinGW: ${OPTION_MINGW}")
if (OPTION_MINGW)
    message(STATUS "MinGW toolchain file: '${CMAKE_TOOLCHAIN_FILE}'")
endif ()
message(STATUS "Option to include NextSubvolume: ${OPTION_NSV}")
message(STATUS "Option to include VTK: ${OPTION_VTK}")
message(STATUS "Option to compile with static libraries: ${OPTION_STATIC}")
message(STATUS "Option to include OpenGL support: ${OPTION_USE_OPENGL}")
message(STATUS "Option to include LibTiff: ${OPTION_USE_LIBTIFF}")
message(STATUS "Option to include Zlib: ${OPTION_USE_ZLIB}")
message(STATUS "Option to incude Python module: ${OPTION_PYTHON}")
message(STATUS "Option to run Libsmoldyn examples: ${OPTION_EXAMPLES}")
message(STATUS "Option to build documentation: ${OPTION_DOCS}")



####### Compiler flags ######################
#
# Must use a C++14 compatible compiler (gcc>=5.x).
# NOTE: C++17 has parallel implmenetation of many STL algorithms. It would be
# nice to use them. Most compilers support C++17 these days.
#
set(CMAKE_C_STANDARD 11)

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# macro to check if a flag exists and enable for compiler.
function(CheckCompilerFlagAndAdd)
    check_c_compiler_flag(${ARGV0} C_FLAG_EXISTS)
    check_cxx_compiler_flag(${ARGV0} CXX_FLAG_EXISTS)
    if(C_FLAG_EXISTS OR CXX_FLAG_EXISTS)
        if(${ARGV1})
            add_definitions(${ARGV0}=${ARGV1})
        else()
            add_definitions(${ARGV0})
        endif()
        message(STATUS "[COMPILER] Adding ${ARGV0}")
    else()
        message(STATUS "Compiler does not support ${ARGV0}. Ignoring..")
    endif()
endfunction()

if(MSVC)
    # FIXME: On Windows, when using MSVC, reduce the amount of warnings. MSVC
    # generates a lot of warning about casting which pollute the build log and
    # makes it hard to find the error.
    add_compile_options(/W1)
else()
    # enable all warnings.
    add_compile_options(-Wall -Wextra)
    CheckCompilerFlagAndAdd(-Wno-unused-parameter)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the buid type: None, Debug, Release, RelWithDebInfo, or MinSizeRel" FORCE)
endif ()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(OPTION_STRICT_BUILD)
    add_definitions(-Wextra -Werror)
    CheckCompilerFlagAndAdd(-Wno-format-truncation)
    CheckCompilerFlagAndAdd(-Wno-uninitialized)
    CheckCompilerFlagAndAdd(-Wno-unused-parameter)
    CheckCompilerFlagAndAdd(-Wno-unused-but-set-parameter)
endif()

# We need a c++14 support to build python bindings.
if(OPTION_PYTHON)
    set(CMAKE_CXX_STANDARD 14)
    try_compile(COMPILER_SUPPORT_C14
        ${CMAKE_BINARY_DIR}/_test_compiler
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_compiler.cpp
        OUTPUT_VARIABLE COMPILER_TEST_OUTPUT
        )
    if(NOT COMPILER_SUPPORT_C14)
        message(STATUS "Failed to build test program: ${COMPILER_TEST_OUTPUT}")
        message(FATAL_ERROR "Your compiler does not support C++14. "
            "Please use a C++14 compliant compiler. "
            "See https://en.cppreference.com/w/cpp/compiler_support/14 "
            "for C++14 support among C++ compilers.")
    endif()
endif()


######### Core code information ###########

#
# Source files for main target smoldyn and libsmodyn. Also these sources are
# recompiled for python module _smoldyn.*.so with a slightly different set of
# flags. Hence, a list. This list is used in source/python/CMakeLists.txt file
# as well.
#
# TODO: May be a good idea to create libSmoldyn and link it statically to the
# python module and smoldyn binary. This way we don't have to compile it
# twice.
#
set(SRC_FILES
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smolbng.c
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smolboxes.c
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smolcmd.c
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smolcomparts.c
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smolfilament.c
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smolgraphics.c
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smolmolec.c
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smolport.c
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smollattice.c
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smolreact.c
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smolrule.c
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smolsim.cpp
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smolsurface.c
	${CMAKE_SOURCE_DIR}/source/Smoldyn/smolwall.c
	)

set(MAIN_FILES ${CMAKE_SOURCE_DIR}/source/Smoldyn/smoldyn.cpp)

include_directories(source/libSteve source/Smoldyn ${CMAKE_BINARY_DIR})

if(OPTION_VCELL)
    list(APPEND SRC_FILES
        ${CMAKE_SOURCE_DIR}/source/Smoldyn/smoldynhybrid.c
        ${CMAKE_SOURCE_DIR}/source/vcell/SimpleMesh.cpp
        ${CMAKE_SOURCE_DIR}/source/vcell/SimpleValueProvider.cpp )
    include_directories(source/vcell)
endif(OPTION_VCELL)

if(OPTION_TARGET_LIBSMOLDYN)
    list(APPEND SRC_FILES ${CMAKE_SOURCE_DIR}/source/Smoldyn/libsmoldyn.cpp)
endif(OPTION_TARGET_LIBSMOLDYN)

if(OPTION_USE_LIBTIFF AND NOT OPTION_USE_OPENGL)
    message(WARNING "LibTiff won't be used because build does not include OpenGL.")
    set(OPTION_USE_LIBTIFF OFF)
endif()

if(OPTION_ASAN)
    # Address sanitizer
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif()


set(VCELL_BUILD OFF)
set(APPLE_BUILD OFF)
set(NIX_BUILD OFF)
set(MINGW_BUILD OFF)
set(WINDOWS_BUILD OFF)

if (OPTION_VCELL)
    set(VCELL_BUILD ON)
    message(STATUS "Compiling for VCell")
    add_compile_options(-D_CRT_SECURE_NO_WARNINGS)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/source/vcell)
elseif (OPTION_MINGW)
    set(MINGW_BUILD ON)
    message(STATUS "Cross-compiling for Windows using MinGW libraries from: ${MINGWDIR}")
    set(BNG2_PATH "%PROGRAMFILES%\\\\Smoldyn\\\\BioNetGen\\\\BNG2.pl")
    include_directories(${MINGWDIR}/include)
    link_directories(${MINGWDIR}/lib)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D FREEGLUT_STATIC -static")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D FREEGLUT_STATIC -static")
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static -s")
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static -s")
elseif (APPLE)
    set(APPLE_BUILD ON)
    message(STATUS "Compiling on an Apple computer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated -mmacosx-version-min=10.9")
    set(BNG2_PATH "/usr/local/bin/BioNetGen/BNG2.pl")
elseif (WIN32)
    set(WINDOWS_BUILD ON)
    if(MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif()

    message(STATUS "Compiling on a Windows computer")
    set(BNG2_PATH "%PROGRAMFILES%\\\\Smoldyn\\\\BioNetGen\\\\BNG2.pl")
    if (OPTION_VTK)
        message(WARNING "VTK not included because it isn't supported for Windows build")
        set(OPTION_VTK OFF)
    endif ()
else ()
    set(NIX_BUILD ON)
    message(STATUS "Compiling on a Linux computer")
    set(BNG2_PATH "/usr/local/bin/BioNetGen/BNG2.pl")
endif ()

message(STATUS "CMAKE_CXX_FLAGS variable: '${CMAKE_CXX_FLAGS}'")
message(STATUS "BioNetGen path: '${BNG2_PATH}' ")


####### Option: Build with OpenGL (gl and glu only, not glut) ##########

if (OPTION_USE_OPENGL)
    # see cmake --help-policy CMP0072"
    set(OpenGL_GL_PREFERENCE LEGACY)

    if (VCELL_BUILD)
        set(HAVE_OPENGL TRUE)
        set(HAVE_GL_GL_H TRUE)
        set(HAVE_GL_GLU_H TRUE)
    elseif (MINGW_BUILD)
        list(APPEND DEP_LIBS opengl32 glu32 freeglut_static gdi32 winmm)
        find_file(HAVE_GL_GL_H GL/gl.h PATHS ${MINGWDIR}/include)
        find_file(HAVE_OPENGL_GL_H OpenGL/gl.h PATHS ${MINGWDIR}/include)
        find_file(HAVE_GL_GLU_H GL/glu.h PATHS ${MINGWDIR}/include)
        find_file(HAVE_OPENGL_GLU_H OpenGL/glu.h PATHS ${MINGWDIR}/include)

        find_path(OPENGL_INCLUDE_DIR gl.h PATHS ${MINGWDIR}/include/GL)
        get_filename_component(OPENGL_INCLUDE_DIR "${HAVE_GL_GL_H}" PATH)
        find_library(OPENGL_LIBRARIES libopengl32.a PATHS ${MINGWDIR}/lib)
        find_library(GLU32_LIBRARIES libglu32.a PATHS ${MINGWDIR}/lib)
        if(GLU32_LIBRARIES)
            message(STATUS "Glu32 found: '${GLU32_LIBRARIES}'")
        else()
            message(FATAL_ERROR "Glu32 not found.  Building without OpenGL is possible.")
        endif()
    elseif (WINDOWS_BUILD)
        find_file(HAVE_GL_GL_H GL/gl.h PATHS source/MSVClibs NO_DEFAULT_PATH)
        find_file(HAVE_GL_GLU_H GL/glu.h PATHS source/MSVClibs NO_DEFAULT_PATH)
        include(FindOpenGL)
        find_path(OPENGL_INCLUDE_DIR GL/gl.h source/MSVClibs NO_DEFAULT_PATH)
    else()
        find_file(HAVE_GL_GL_H GL/gl.h)
        find_file(HAVE_OPENGL_GL_H OpenGL/gl.h)
        find_file(HAVE_GL_GLU_H GL/glu.h)
        find_file(HAVE_OPENGL_GLU_H OpenGL/glu.h)
        include(FindOpenGL)
    endif()

    if(OPENGL_INCLUDE_DIR AND OPENGL_LIBRARIES)
        set(OPENGL_FOUND TRUE)
    endif()

    if(OPENGL_FOUND)
        message(STATUS "OpenGL found: '${OPENGL_INCLUDE_DIR}', '${OPENGL_LIBRARIES}'")
    else()
        message(STATUS "OpenGL include and library variables: '${OPENGL_INCLUDE_DIR}', '${OPENGL_LIBRARIES}'")
        message(FATAL_ERROR "OpenGL not found.  Building without OpenGL is possible.")
    endif()

    set(HAVE_OPENGL TRUE)
    include_directories(${OPENGL_INCLUDE_DIR})
    list(APPEND DEP_LIBS ${OPENGL_LIBRARIES} ${GLU32_LIBRARIES})
endif()


####### Option: Build with OpenGL (glut only) ##########

if (OPTION_USE_OPENGL)
    if (VCELL_BUILD)
        set(HAVE_GL_GLUT_H TRUE)
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/windows/glut-3.7.6)
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/windows/glut-3.7.6)
        if (ARCH_64bit)
            list(APPEND DEP_LIBS glut64)
        else()
            list(APPEND DEP_LIBS glut32 zlib)
        endif()
    elseif (MINGW_BUILD)
        add_compile_definitions(FREEGLUT_STATIC)
        find_file(HAVE_GL_GLUT_H GL/glut.h PATHS ${MINGWDIR}/include)
        find_path(GLUT_INCLUDE_DIR glut.h PATHS ${MINGWDIR}/include/GL)
        find_library(GLUT_LIBRARIES libfreeglut_static.a PATHS ${MINGWDIR}/lib)
    elseif (WINDOWS_BUILD)
        add_compile_definitions(FREEGLUT_STATIC)
        find_path(GLUT_INCLUDE_DIR GL/glut.h source/MSVClibs NO_DEFAULT_PATH)
        set(HAVE_GL_GLUT_H TRUE)
        find_file(GLUT_LIBRARIES NAMES freeglut_static.lib PATHS source/MSVClibs NO_DEFAULT_PATH)
        set(GLUT_FOUND TRUE)
    else ()
        find_file(HAVE_GL_GLUT_H GL/glut.h)
        find_file(HAVE_GLUT_GLUT_H GLUT/glut.h)
        include(FindGLUT)
    endif ()

    if (GLUT_INCLUDE_DIR AND GLUT_LIBRARIES)
        set(GLUT_FOUND TRUE)
    endif ()

    if(GLUT_FOUND)
        message(STATUS "Glut found: '${GLUT_INCLUDE_DIR}', '${GLUT_LIBRARIES}'")
    else()
        message(STATUS "Glut include and library variables: '${GLUT_INCLUDE_DIR}', '${GLUT_LIBRARIES}'")
        message(FATAL_ERROR "Glut not found.  Building without OpenGL is possible.")
    endif()

    include_directories(${GLUT_INCLUDE_DIR})
    list(APPEND DEP_LIBS ${GLUT_LIBRARIES})
endif()


####### Build with LibX11 (only required for static build on Mac) ##########

#if (OPTION_STATIC AND OPTION_USE_OPENGL AND HAVE_OPENGL)
#	   find_package(X11)
#	   if (X11_FOUND)
#        message(STATUS "Found X11: '${X11_INCLUDE_DIR}', '${X11_LIBRARIES}'")
#    else ()
#        message(FATAL_ERROR "X11 not found")
#    endif ()
#    include_directories(${X11_INCLUDE_DIR})
#    list(APPEND DEP_LIBS ${X11_LIBRARIES})
#endif()


####### Option: Build with LibTiff ##########

if (OPTION_USE_LIBTIFF)
    if (MINGW_BUILD)
        find_path(TIFF_INCLUDE_DIR tiff.h PATHS ${MINGWDIR}/include NO_DEFAULT_PATH)
        find_library(TIFF_LIBRARY libtiff.a PATHS ${MINGWDIR}/lib NO_DEFAULT_PATH)
    elseif (WINDOWS_BUILD)
        find_path(TIFF_INCLUDE_DIR tiff.h source/MSVClibs NO_DEFAULT_PATH)
        set(HAVE_TIFF_H TRUE)
        find_file(TIFF_LIBRARY NAMES tiff.lib PATHS source/MSVClibs NO_DEFAULT_PATH)
        set(TIFF_FOUND TRUE)
    elseif (OPTION_STATIC)
        find_path(TIFF_INCLUDE_DIR tiff.h PATHS ${CMAKE_SOURCE_DIR}/Mac/libtiff/include NO_DEFAULT_PATH)
        find_library(TIFF_LIBRARY libtiff.a ${CMAKE_SOURCE_DIR}/Mac/libtiff/lib NO_DEFAULT_PATH)
    else ()
        find_package(TIFF)
    endif()

    if(TIFF_INCLUDE_DIR AND TIFF_LIBRARY)
        set(HAVE_LIBTIFF TRUE)
        message(STATUS "Found Libtiff: '${TIFF_INCLUDE_DIR}', '${TIFF_LIBRARY}'")
    else()
        set(HAVE_LIBTIFF FALSE)
        message(STATUS "Libtiff include and library variables: '${TIFF_INCLUDE_DIR}', '${TIFF_LIBRARY}'")
        message(FATAL_ERROR "Libtiff not found")
    endif ()

    include_directories(${TIFF_INCLUDE_DIR})
    list(APPEND DEP_LIBS ${TIFF_LIBRARY})
endif ()


####### Option: Build with Zlib ##########

if(OPTION_USE_ZLIB)
    if(OPTION_MINGW)
        find_path(ZLIB_INCLUDE_DIRS zlib.h PATHS ${MINGWDIR}/include NO_DEFAULT_PATH)
        find_library(ZLIB_LIBRARIES libz.a PATHS ${MINGWDIR}/lib NO_DEFAULT_PATH)
    elseif(OPTION_STATIC)
        find_path(ZLIB_INCLUDE_DIRS zlib.h)
        find_library(ZLIB_LIBRARIES libz.a)
    else()
        include(FindZlib)
    endif()

    if(ZLIB_INCLUDE_DIRS AND ZLIB_LIBRARIES)
        set(HAVE_ZLIB TRUE)
        message(STATUS "Found Zlib: '${ZLIB_INCLUDE_DIRS}', '${ZLIB_LIBRARIES}'")
    else()
        set(HAVE_ZLIB FALSE)
        message(FATAL_ERROR "Zlib not found")
    endif()

    include_directories(${ZLIB_INCLUDE_DIRS})
    list(APPEND DEP_LIBS ${ZLIB_LIBRARIES})
endif(OPTION_USE_ZLIB)


####### Option: Build with NextSubvolume ##########

if (OPTION_NSV)
    add_subdirectory(source/NextSubVolume)
    include_directories(source/NextSubVolume)
endif (OPTION_NSV)

if(OPTION_VTK)
    add_definitions(-DHAVE_VTK)
    find_package(VTK REQUIRED)
    include(${VTK_USE_FILE})

    message(STATUS "VTK_VERSION: ${VTK_VERSION}")
    if(VTK_VERSION VERSION_GREATER_EQUAL "8.0.0")
        message(FATAL_ERROR "Only vtk version <8.0 is supported")
    endif()
    list(APPEND DEP_LIBS ${VTK_LIBRARIES})
endif()


####### Targets ##########
add_subdirectory(source/libSteve)

# Static library is always built. We link it to all other targets.
# OPTION_TARGET_LIBSMOLDYN now only control the shared library.
add_library(smoldyn_static STATIC ${SRC_FILES}
    $<TARGET_OBJECTS:Steve>
    $<TARGET_OBJECTS:nsv>)

set_property(TARGET smoldyn_static PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(smoldyn_static PUBLIC ${DEP_LIBS})
set_target_properties(smoldyn_static PROPERTIES PREFIX "lib")

if (OPTION_TARGET_LIBSMOLDYN)
    add_library(smoldyn_shared SHARED ${SRC_FILES}
        $<TARGET_OBJECTS:Steve>
        $<TARGET_OBJECTS:nsv>)

    target_link_libraries(smoldyn_shared PUBLIC ${DEP_LIBS})

    if (APPLE_BUILD)
        set_target_properties(smoldyn_shared PROPERTIES LINK_FLAGS "-framework Glut -framework OpenGL")
    elseif (WINDOWS_BUILD)
        set_target_properties(smoldyn_shared PROPERTIES PREFIX "lib")
    endif ()
endif ()

if (OPTION_TARGET_SMOLDYN)
    add_executable(smoldyn ${MAIN_FILES})
    target_link_libraries(smoldyn PUBLIC smoldyn_static)
    target_link_libraries(smoldyn PUBLIC ${DEP_LIBS})
endif ()



########## Python module #########

if (OPTION_PYTHON)
    # Find python before using pybind11. There is a possibility of version mismatch later.
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    set(PYBIND11_SOURCE_DIR ${CMAKE_SOURCE_DIR}/source/pybind11)
    add_subdirectory(${PYBIND11_SOURCE_DIR})
    add_subdirectory(${CMAKE_SOURCE_DIR}/source/python)
endif ()


########## install ###########

if(NOT OPTION_MINGW)

    if (OPTION_TARGET_SMOLDYN)
        install(TARGETS smoldyn RUNTIME DESTINATION bin)
    endif()

    if (OPTION_TARGET_LIBSMOLDYN)
        install(TARGETS smoldyn_shared LIBRARY DESTINATION lib)
        install(TARGETS smoldyn_static ARCHIVE DESTINATION lib)
        install(FILES source/Smoldyn/libsmoldyn.h source/Smoldyn/smoldyn.h
            ${CMAKE_CURRENT_BINARY_DIR}/smoldynconfigure.h
            DESTINATION include)
    endif ()
endif ()


########## configure file #########
configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/source/smoldynconfigure.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/smoldynconfigure.h
    )

######### docs ##############
if(OPTION_DOCS)
    add_subdirectory(docs)
endif(OPTION_DOCS)

########## package #########
include(CPack)


######### Testing #############
enable_testing()
if (OPTION_TARGET_LIBSMOLDYN AND OPTION_EXAMPLES)
    add_subdirectory(examples)
endif ()

