add_definitions(-DHAVE_VTK)
find_package(VTK REQUIRED)
message(STATUS "VTK_VERSION: ${VTK_VERSION}")
include_directories(${CMAKE_SOURCE_DIR}/source/vtk)
if(VTK_VERSION VERSION_GREATER_EQUAL "9.0.0")
    message(FATAL_ERROR "Only vtk version <8.x is supported")
endif()

add_library(vtkwrapper STATIC vtkwrapper.cpp)
set_target_properties(vtkwrapper POSITION_INDEPENDENT_CODE ON)


if(OPTION_VTK)
    include(${VTK_USE_FILE})
endif()
