file(GLOB SRC_FILES "*.cpp")
file(GLOB HEADER_FILES "*.h")

if(NOT HAVE_VTK)
	add_definitions(-DHAVE_VTK)
	find_package(VTK REQUIRED)
endif()

#FIND_PACKAGE(Boost REQUIRED)
include_directories(${VTK_INCLUDE_DIRS})

if(OPTION_STATIC)
	add_library(vtkwrapper STATIC ${SRC_FILES} ${HEADER_FILES})
else()
	add_library(vtkwrapper SHARED ${SRC_FILES} ${HEADER_FILES})
endif(OPTION_STATIC)

TARGET_LINK_LIBRARIES(vtkwrapper ${VTK_LIBRARIES})

