# CMakeLists.txt file to build SmolEmulate using CMake.
# Written by Steve Andrews, June 21, 2016.
# This file, and other files of the Smoldyn project, are licensed with LGPL license.
# The primary Smoldyn web site is http://www.smoldyn.org.
# Please report bugs and problems to support@smoldyn.org.

########## Basic setup ##########

project(SmolEmulate)
cmake_minimum_required(VERSION 3.0)
set(SMOLEMULATE_VERSION "1.0")


######### Core code information ###########

set(HEADER_FILES
	${CMAKE_SOURCE_DIR}/../libSteve/Geometry.h
	${CMAKE_SOURCE_DIR}/../libSteve/gnuPipe.h
	${CMAKE_SOURCE_DIR}/../libSteve/math2.h
	${CMAKE_SOURCE_DIR}/../libSteve/random2.h
	${CMAKE_SOURCE_DIR}/../libSteve/Rn.h
	${CMAKE_SOURCE_DIR}/../libSteve/RnSort.h
	${CMAKE_SOURCE_DIR}/../libSteve/rxnparam.h
	${CMAKE_SOURCE_DIR}/../libSteve/rxn2Dparam.h
	${CMAKE_SOURCE_DIR}/../libSteve/string2.h
	${CMAKE_SOURCE_DIR}/../libSteve/SurfaceParam.h
	${CMAKE_SOURCE_DIR}/../libSteve/SFMT/SFMT.h
)

set(SRC_FILES
	${CMAKE_SOURCE_DIR}/../libSteve/Geometry.c
	${CMAKE_SOURCE_DIR}/../libSteve/gnuPipe.c
	${CMAKE_SOURCE_DIR}/../libSteve/math2.c
	${CMAKE_SOURCE_DIR}/../libSteve/random2.c
	${CMAKE_SOURCE_DIR}/../libSteve/Rn.c
	${CMAKE_SOURCE_DIR}/../libSteve/RnSort.c
	${CMAKE_SOURCE_DIR}/../libSteve/rxnparam.c
	${CMAKE_SOURCE_DIR}/../libSteve/rxn2Dparam.c
	${CMAKE_SOURCE_DIR}/../libSteve/string2.c
	${CMAKE_SOURCE_DIR}/../libSteve/SurfaceParam.c
	${CMAKE_SOURCE_DIR}/../libSteve/SFMT/SFMT.c
)

set(MAIN_FILES SmolEmulate.c)

set_source_files_properties(${SRC_FILES} PROPERTIES LANGUAGE C )
set_source_files_properties(${MAIN_FILES} PROPERTIES LANGUAGE C )

include_directories(${CMAKE_SOURCE_DIR}/../libSteve)

list(APPEND DEP_LIBS m)

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

add_executable(SmolEmulate ${SRC_FILES} ${MAIN_FILES} ${HEADER_FILES})
target_link_libraries(SmolEmulate ${DEP_LIBS})


########## install ###########

install(TARGETS SmolEmulate RUNTIME DESTINATION bin)


