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
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/StructuredGrid.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/Species.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/ReactionEquation.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/NextSubvolumeMethod.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/Kairos.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/Io.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/Geometry.cpp
    ${CMAKE_SOURCE_DIR}/source/NextSubVolume/nsvc.cpp
    )

if(OPTION_VTK)
    add_definitions(-DHAVE_VTK)
    find_package(VTK REQUIRED)
    message(STATUS "-- VTK_VERSION: ${VTK_VERSION}")
    include_directories(${CMAKE_SOURCE_DIR}/source/vtk)
    list(APPEND NSV_SRC_FILES ${CMAKE_SOURCE_DIR}/source/vtk/vtkwrapper.cpp)
endif(OPTION_VTK)

if(VTK_VERSION VERSION_GREATER_EQUAL "8.0.0")
    message(FATAL_ERROR "Only vtk version 7.x is supported")
endif()

include(${VTK_USE_FILE})
add_library(nsv STATIC ${NSV_SRC_FILES})
set_target_properties(nsv PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(nsv PRIVATE ${VTK_LIBRARIES})

#vtk_module_autoinit(TARGETS nsv MODULES ${VTK_LIBRARIES})
