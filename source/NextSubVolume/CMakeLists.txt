if (OPTION_MINGW)
    find_path(SIGNAL_H_DIR signal.h PATHS ${MINGWDIR}/include NO_DEFAULT_PATH)
    if (SIGNAL_H_DIR)
        message(STATUS "signal.h found: '${SIGNAL_H_DIR}'")
    else ()
        message(FATAL_ERROR "signal.h not found")
    endif ()
    include_directories(${SIGNAL_H_DIR})
endif ()

set(Boost_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/boost_include)
include_directories(SYSTEM ${Boost_INCLUDE_DIR})
message(STATUS "Boost headers found: '${Boost_INCLUDE_DIR}'")

set(SRC_FILES
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/StructuredGrid.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/Species.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/ReactionEquation.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/NextSubvolumeMethod.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/Kairos.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/Io.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/Geometry.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/nsvc.cpp
    )

add_library(nsv STATIC ${SRC_FILES})
set_target_properties(nsv PROPERTIES POSITION_INDEPENDENT_CODE ON)
