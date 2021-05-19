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

set(NSV_SRC_FILES
    StructuredGrid.cpp
    Species.cpp
    ReactionEquation.cpp
    NextSubvolumeMethod.cpp
    Kairos.cpp
    Io.cpp
    Geometry.cpp
    nsvc.cpp
    )


if(OPTION_VTK)
    add_definitions(-DHAVE_VTK)
    find_package(VTK REQUIRED)
    include(${VTK_USE_FILE})
    message(STATUS "VTK_VERSION: ${VTK_VERSION}")
    if(VTK_VERSION VERSION_GREATER_EQUAL "8.0.0")
        message(FATAL_ERROR "Only vtk version <8.0 is supported")
    endif()
    list(APPEND NSV_SRC_FILES vtkwrapper.cpp)
endif()

add_library(nsv OBJECT ${NSV_SRC_FILES})
set_target_properties(nsv PROPERTIES POSITION_INDEPENDENT_CODE ON)
