# CMakeLists.txt file to build wrl2smol using CMake.
# Written by Steve Andrews, July 26, 2012.
# This file, and other files of the Smoldyn project, are licensed with LGPL license.
# The primary Smoldyn web site is http://www.smoldyn.org.
# Please report bugs and problems to support@smoldyn.org.

########## Basic setup ##########

project(wrl2smol)
cmake_minimum_required(VERSION 2.6)
set(WRL2SMOL_VERSION "1.0")


######### Core code information ###########

set(MAIN_FILES wrl2smol.c)

set_source_files_properties(${MAIN_FILES} PROPERTIES LANGUAGE C )


####### Build for debugging or release ##########

if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS_DEBUG "-O0 -g -Wall")
	set(CMAKE_C_FLAGS_RELEASE "-O2 -Wall")
endif(CMAKE_COMPILER_IS_GNUCC)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the buid type: None, Debug, Release, RelWithDebInfo, or MinSizeRel" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


####### Targets ##########


add_executable(wrl2smol ${MAIN_FILES})


########## install ###########

install(TARGETS wrl2smol RUNTIME DESTINATION bin)


