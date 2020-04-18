/*
 * Python module.
 * Author: Dilawar Singh <dilawar.s.rajput@gmail.com>
 */

#include <pybind11/pybind11.h>

#include "../Smoldyn/smoldynfuncs.h"

namespace py = pybind11;

PYBIND11_MODULE(smoldyn, m) {
    m.doc() = R"pbdoc(
        smoldyn
        -----------------------
    )pbdoc";

    py::class_<simstruct>(m, "simstruct")
        .def(py::init<>())
        ;

    /* Function */
    m.def("simulate", &simInitAndLoad);

    m.attr("__version__") = SMOLDYN_VERSION;

}
