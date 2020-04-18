/*
 * Python module.
 * Author: Dilawar Singh <dilawar.s.rajput@gmail.com>
 */

#include <string>
using namespace std;

#include <pybind11/pybind11.h>

#include "../Smoldyn/smoldynfuncs.h"

namespace py = pybind11;

int simulate(const string& filepath, const string& flags) 
{
    string filename, fileroot;
    auto pos = filepath.find_last_of('/');

    fileroot = filepath.substr(0, pos);
    filename = filepath.substr(pos+1);

    simptr sim;
    return simInitAndLoad(filename.c_str(), fileroot.c_str(), &sim, flags.c_str());
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
