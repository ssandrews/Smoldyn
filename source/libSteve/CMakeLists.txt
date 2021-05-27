set(SRC_FILES
    Geometry.c
    List.c
    math2.c
    opengl2.c
    parse.c
    queue.c
    random2.c
    Rn.c
    RnSort.c
    rxnparam.c
    SimCommand.c
    Sphere.c
    string2.c
    SurfaceParam.c
    Zn.c
    SFMT/SFMT.c)

add_library(Steve OBJECT ${SRC_FILES})
set_target_properties(Steve PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(Steve PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Smoldyn
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(Steve PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(OPENGL_FOUND)
    list(APPEND DEP_LIBS ${OPENGL_LIBRARIES})
endif()
if(TIFF_FOUND)
    list(APPEND DEP_LIBS ${TIFF_LIBRARY})
endif()
