# python bindings
find_package(pybind11 REQUIRED)
pybind11_add_module(smoldyn MODULE pysmoldyn.cpp)
