/*
 * Python module.
 * Author: Dilawar Singh <dilawar.s.rajput@gmail.com>
 */

#include <pybind11/pybind11.h>

#include "../Smoldyn/smoldynfuncs.h"

namespace py = pybind11;

int simulate(const char* filename, const char* fileroot, const char* flags) 
{
    simptr sim;
    return simInitAndLoad(filename, fileroot, &sim, flags);
}

PYBIND11_MODULE(smoldyn, m) {
    m.doc() = R"pbdoc(
        smoldyn
        -----------------------
    )pbdoc";

    /* Function */
    m.def("simulate", &simulate);

    m.attr("__version__") = SMOLDYN_VERSION;

}
