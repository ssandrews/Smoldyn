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

int simulate(const string& filepath, const string& flags) 
{
    int i, er, pflag, wflag, tflag, Vflag, oflag;

    string filename, fileroot;
    auto pos = filepath.find_last_of('/');

    fileroot = filepath.substr(0, pos+1);
    filename = filepath.substr(pos+1);

    simptr sim;
    cout << "root:" << fileroot << " name:"<<filename << " flags="<< flags<<endl;

    sim = NULL;
    cout << "root="<< fileroot << " fname=" << filename << ", flags=" << flags 
        << endl; 
#ifdef OPTION_VCELL
    er = simInitAndLoad(fileroot.c_str(), filename.c_str(), &sim, flags.c_str(),
            new SimpleValueProviderFactory(), new SimpleMesh());
#else
    er = simInitAndLoad(fileroot.c_str(), filename.c_str(), &sim, flags.c_str());
#endif
    if (!er) {
        // if (!tflag && sim->graphss && sim->graphss->graphics != 0)
        //     gl2glutInit(&argc, argv);
        er = simUpdateAndDisplay(sim);
    }
    if (!oflag && !pflag && !er)
        er = scmdopenfiles((cmdssptr)sim->cmds, wflag);
    if (pflag || er) {
        simLog(sim, 4, "%sSimulation skipped\n", er ? "\n" : "");
    }
    else {
        fflush(stdout);
        fflush(stderr);
        if (tflag || !sim->graphss || sim->graphss->graphics == 0) {
            er = smolsimulate(sim);
            endsimulate(sim, er);
        }
        else {
            smolsimulategl(sim);
        }
    }
    simfree(sim);
    simfuncfree();
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
