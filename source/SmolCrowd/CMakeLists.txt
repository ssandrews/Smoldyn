# CMakeLists.txt file to build SmolCrowd using CMake.
# Written by Steve Andrews, July 26, 2012.
# This file, and other files of the Smoldyn project, are licensed with LGPL license.
# The primary Smoldyn web site is http://www.smoldyn.org.
# Please report bugs and problems to support@smoldyn.org.

########## Basic setup ##########

project(SmolCrowd)
cmake_minimum_required(VERSION 2.6)
set(SMOLCROWD_VERSION "1.0")


######### Core code information ###########

set(HEADER_FILES
	../lib/Geometry.h
	../lib/math2.h
	../lib/random2.h
	../lib/Rn.h
	../lib/RnSort.h
	../lib/SFMT/SFMT.h
)

set(SRC_FILES
	../lib/Geometry.c
	../lib/math2.c
	../lib/random2.c
	../lib/Rn.c
	../lib/RnSort.c
	../lib/SFMT/SFMT.c
)

set(MAIN_FILES SmolCrowd.c)

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

add_executable(SmolCrowd ${SRC_FILES} ${MAIN_FILES} ${HEADER_FILES})
target_link_libraries(SmolCrowd ${DEP_LIBS})


########## install ###########

install(TARGETS SmolCrowd RUNTIME DESTINATION bin)


