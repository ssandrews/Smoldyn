/*
 * Python module.
 * Author: Dilawar Singh <dilawar.s.rajput@gmail.com>
 */

#include <iostream>
#include <string>
using namespace std;

#include <pybind11/pybind11.h>

#include "../Smoldyn/smoldynfuncs.h"

namespace py = pybind11;

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  Initialize and run the simulation. This function takes input file
 * and run the simulation. This is more or less same as `main()` function in
 * smoldyn.c file.
 *
 * @Param filepath
 * @Param flags
 *
 * @Returns   
 */
/* ----------------------------------------------------------------------------*/
int init_and_run(const string& filepath, const string& flags) 
{
    int er = 0, wflag;

    string filename, fileroot;
    auto pos = filepath.find_last_of('/');

    fileroot = filepath.substr(0, pos+1);
    filename = filepath.substr(pos+1);

    simptr sim = nullptr;
#ifdef OPTION_VCELL
    er = simInitAndLoad(fileroot.c_str(), filename.c_str(), &sim, flags.c_str(),
            new SimpleValueProviderFactory(), new SimpleMesh());
#else
    er = simInitAndLoad(fileroot.c_str(), filename.c_str(), &sim, flags.c_str());
#endif
    if (!er) {
        // if (!tflag && sim->graphss && sim->graphss->graphics != 0)
            // gl2glutInit(0, "");
        er = simUpdateAndDisplay(sim);
    }
    if(!er)
        er = scmdopenfiles((cmdssptr)sim->cmds, wflag);
    if (er) {
        simLog(sim, 4, "%sSimulation skipped\n", er ? "\n" : "");
    }
    else {
        fflush(stdout);
        fflush(stderr);
        if (!sim->graphss || sim->graphss->graphics == 0) {
            er = smolsimulate(sim);
            endsimulate(sim, er);
        }
        else {
            smolsimulategl(sim);
        }
    }
    simfree(sim);
    simfuncfree();
    return er;
}


PYBIND11_MODULE(_smoldyn, m) {
    m.doc() = R"pbdoc(
        smoldyn
        -----------------------
    )pbdoc";

    /* simulation pointer */
    unique_ptr<simstruct> simptr{new simstruct()};

    /* Function */
    m.def("load_model", &init_and_run, "Load model from a txt file");

    m.attr("__version__") = SMOLDYN_VERSION;

}
