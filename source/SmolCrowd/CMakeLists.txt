# CMakeLists.txt file to build SmolCrowd using CMake.
# Written by Steve Andrews, July 26, 2012.
# This file, and other files of the Smoldyn project, are licensed with LGPL license.
# The primary Smoldyn web site is http://www.smoldyn.org.
# Please report bugs and problems to support@smoldyn.org.

########## Basic setup ##########

project(SmolCrowd)
cmake_minimum_required(VERSION 3.0)
if (NOT SMOLCROWD_VERSION)
    set(SMOLCROWD_VERSION "1.0")
endif ()
message(STATUS "SmolCrowd version set to: '${SMOLCROWD_VERSION}'")


######### Core code information ###########

set(HEADER_FILES
 ../libSteve/Geometry.h
 ../libSteve/math2.h
 ../libSteve/random2.h
 ../libSteve/Rn.h
 ../libSteve/RnSort.h
 ../libSteve/SFMT/SFMT.h
)

set(SRC_FILES
 ../libSteve/Geometry.c
 ../libSteve/math2.c
 ../libSteve/random2.c
 ../libSteve/Rn.c
 ../libSteve/RnSort.c
 ../libSteve/SFMT/SFMT.c
)

set(MAIN_FILES SmolCrowd.c)

set_source_files_properties(${SRC_FILES} PROPERTIES LANGUAGE C )
set_source_files_properties(${MAIN_FILES} PROPERTIES LANGUAGE C )

include_directories(../libSteve)

list(APPEND DEP_LIBS m)


####### Compiler flags ##########

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
  "Choose the buid type: None, Debug, Release, RelWithDebInfo, or MinSizeRel" FORCE)
endif ()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

####### Targets ##########

add_executable(SmolCrowd ${SRC_FILES} ${MAIN_FILES} ${HEADER_FILES})
target_link_libraries(SmolCrowd ${DEP_LIBS})


########## install ###########

install(TARGETS SmolCrowd RUNTIME DESTINATION bin)


