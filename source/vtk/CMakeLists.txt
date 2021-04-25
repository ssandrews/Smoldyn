add_definitions(-DHAVE_VTK)

find_package(VTK REQUIRED QUIET)
include(${VTK_USE_FILE})

message(STATUS "VTK_VERSION: ${VTK_VERSION}")
include_directories(${CMAKE_SOURCE_DIR}/source/vtk)
if(VTK_VERSION VERSION_GREATER_EQUAL "9.0.0")
    message(FATAL_ERROR "Only vtk version <8.x is supported")
endif()

add_library(vtkwrapper STATIC vtkwrapper.cpp)

set_target_properties(vtkwrapper PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(vtkwrapper PRIVATE ${VTK_LIBRARIES})
