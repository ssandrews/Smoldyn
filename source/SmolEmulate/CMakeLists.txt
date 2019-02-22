# CMakeLists.txt file to build SmolEmulate using CMake.
# Written by Steve Andrews, June 21, 2016.
# This file, and other files of the Smoldyn project, are licensed with LGPL license.
# The primary Smoldyn web site is http://www.smoldyn.org.
# Please report bugs and problems to support@smoldyn.org.

########## Basic setup ##########

project(SmolEmulate)
cmake_minimum_required(VERSION 2.6)
set(SMOLEMULATE_VERSION "1.0")


######### Core code information ###########

set(HEADER_FILES
	../lib/Geometry.h
	../lib/gnuPipe.h
	../lib/math2.h
	../lib/random2.h
	../lib/Rn.h
	../lib/RnSort.h
	../lib/rxnparam.h
	../lib/rxn2Dparam.h
	../lib/string2.h
	../lib/SurfaceParam.h
	../lib/SFMT/SFMT.h
)

set(SRC_FILES
	../lib/Geometry.c
	../lib/gnuPipe.c
	../lib/math2.c
	../lib/random2.c
	../lib/Rn.c
	../lib/RnSort.c
	../lib/rxnparam.c
	../lib/rxn2Dparam.c
	../lib/string2.c
	../lib/SurfaceParam.c
	../lib/SFMT/SFMT.c
)

set(MAIN_FILES SmolEmulate.c)

set_source_files_properties(${SRC_FILES} PROPERTIES LANGUAGE C )
set_source_files_properties(${MAIN_FILES} PROPERTIES LANGUAGE C )

include_directories(../lib)

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


