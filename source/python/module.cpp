/*
 * Python module.
 * Author: Dilawar Singh <dilawar.s.rajput@gmail.com>
 */

#include <iostream>
#include <string>
#include <map>

using namespace std;

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"
#include "pybind11/functional.h"

#include "Smoldyn.h"
#include "SmoldynSpecies.h"

#include "../Smoldyn/smoldynfuncs.h"

using namespace pybind11::literals;  // for _a

// Defined in SimCommand.c file.
extern int scmdopenfiles(cmdssptr cmds, int overwrite);

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
    int er = 0, wflag = 0;

    string filename, fileroot;
    auto   pos = filepath.find_last_of('/');

    fileroot = filepath.substr(0, pos + 1);
    filename = filepath.substr(pos + 1);

    simptr sim = nullptr;
#ifdef OPTION_VCELL
    er = simInitAndLoad(fileroot.c_str(), filename.c_str(), &sim, flags.c_str(),
        new SimpleValueProviderFactory(), new SimpleMesh());
#else
    er =
        simInitAndLoad(fileroot.c_str(), filename.c_str(), &sim, flags.c_str());
#endif
    if(!er) {
        // if (!tflag && sim->graphss && sim->graphss->graphics != 0)
        // gl2glutInit(0, "");
        er = simUpdateAndDisplay(sim);
    }
    if(!er)
        er = scmdopenfiles((cmdssptr)sim->cmds, wflag);
    if(er) {
        simLog(sim, 4, "%sSimulation skipped\n", er ? "\n" : "");
    }
    else {
        fflush(stdout);
        fflush(stderr);
        if(!sim->graphss || sim->graphss->graphics == 0) {
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

PYBIND11_MODULE(_smoldyn, m)
{
    m.doc() = R"pbdoc(
        smoldyn
        -----------------------
    )pbdoc";

    py::enum_<SrfAction>(m, "SA")
        .value("reflect", SrfAction::SAreflect)
        .value("trans", SrfAction::SAtrans)
        .value("absorb", SrfAction::SAabsorb)
        .value("jump", SrfAction::SAjump)
        .value("port", SrfAction::SAport)
        .value("mult", SrfAction::SAmult)
        .value("no", SrfAction::SAno)
        .value("none", SrfAction::SAnone)
        .value("adsorb", SrfAction::SAadsorb)
        .value("revdes", SrfAction::SArevdes)
        .value("irrevdes", SrfAction::SAirrevdes)
        .value("flip", SrfAction::SAflip);

    py::enum_<MolecState>(m, "MS")
        .value("soln", MolecState::MSsoln)
        .value("front", MolecState::MSfront)
        .value("back", MolecState::MSback)
        .value("up", MolecState::MSup)
        .value("down", MolecState::MSdown)
        .value("bsoln", MolecState::MSbsoln)
        .value("all", MolecState::MSall)
        .value("none", MolecState::MSnone)
        .value("some", MolecState::MSsome);

    py::enum_<PanelFace>(m, "PF")
        .value("front", PanelFace::PFfront)
        .value("back", PanelFace::PFback)
        .value("none", PanelFace::PFnone)
        .value("both", PanelFace::PFboth);

    py::enum_<PanelShape>(m, "PS")
        .value("rect", PanelShape::PSrect)
        .value("tri", PanelShape::PStri)
        .value("sph", PanelShape::PSsph)
        .value("cyl", PanelShape::PScyl)
        .value("hemi", PanelShape::PShemi)
        .value("disk", PanelShape::PSdisk)
        .value("all", PanelShape::PSall)
        .value("none", PanelShape::PSnone);

    /**
     * @Synopsis  Error codes.
     */
    py::enum_<ErrorCode>(m, "ErrorCode")
        .value("ok", ErrorCode::ECok)
        .value("notify", ErrorCode::ECnotify)
        .value("warning", ErrorCode::ECwarning)
        .value("nonexist", ErrorCode::ECnonexist)
        .value("all", ErrorCode::ECall)
        .value("missing", ErrorCode::ECmissing)
        .value("bounds", ErrorCode::ECbounds)
        .value("syntax", ErrorCode::ECsyntax)
        .value("error", ErrorCode::ECerror)
        .value("memory", ErrorCode::ECmemory)
        .value("bug", ErrorCode::ECbug)
        .value("same", ErrorCode::ECsame)
        .value("wildcard", ErrorCode::ECwildcard);

    /* Simulation class */
    py::class_<Smoldyn>(m, "Smoldyn")
        .def(py::init<bool>(), "debug"_a = false)
        .def_property("dim", &Smoldyn::getDim, &Smoldyn::setDim)
        .def_property("seed", &Smoldyn::getRandomSeed, &Smoldyn::setRandomSeed)
        .def_property("bounds", &Smoldyn::getBounds, &Smoldyn::setBounds)
        /* box/molperbox etc */
        .def("setPartitions", &Smoldyn::setPartitions)
        /* Molecules */
        .def("addSpecies", &Smoldyn::addSpecies, "name"_a, "mollist"_a = "")
        .def("setSpeciesStyle", &Smoldyn::setSpeciesStyle)
        .def("setSpeciesMobility", &Smoldyn::setSpeciesMobility, "species"_a,
            "state"_a, "diffConst"_a, "drift"_a = std::vector<double>(),
            "difmatrix"_a = std::vector<double>())
        /* Surface */
        .def("addSurface", &Smoldyn::addSurface, "name"_a)
        .def("setSurfaceAction", &Smoldyn::setSurfaceAction)
        .def("addSurfaceMolecules", &Smoldyn::addSurfaceMolecules)
        /* Panel */
        .def("addPanel", &Smoldyn::addPanel)
        /* Compartment */
        .def("addCompartment", &Smoldyn::addCompartment)
        .def("addCompartmentSurface", &Smoldyn::addCompartmentSurface)
        .def("addCompartmentPoint", &Smoldyn::addCompartmentPoint)
        .def("addComparmentMolecules", &Smoldyn::addCompartmentMolecules)
        /* Reaction */
        .def("addReaction", &Smoldyn::addReaction, "reac"_a, "reactant1"_a,
            "rstate1"_a, "reactant2"_a, "rstate2"_a, "products"_a,
            "productstates"_a, "rate"_a)
        .def("setReactionRegion", &Smoldyn::setReactionRegion)
        .def("setBoundaryType", &Smoldyn::setBoundaryType)
        .def("addMolList", &Smoldyn::addMolList)
        /* data */
        .def("getMoleculeCount", &Smoldyn::getMoleculeCount)
        /* Graphics */
        .def("setGraphicsParams", &Smoldyn::setGraphicsParams)
        /* Simulation */
        .def("setTimes", &Smoldyn::setSimTimes)
        .def("updateSim", &Smoldyn::updateSim)
        .def("runSim", &Smoldyn::runSim, "stoptime"_a, "dt"_a, "display"_a = true)
        .def("runUntil", &Smoldyn::runUntil, "breaktime"_a, "dt"_a = 0.0,
            "display"_a = true)
        .def("addSolutionMolecules", &Smoldyn::addSolutionMolecules)
        .def_property("dt", &Smoldyn::getDt, &Smoldyn::setDt);

    /* Function */
    m.def("load_model", &init_and_run, "filepath"_a, "args"_a = "",
        "Load model from a txt file");

    /* attributes */
    m.attr("__version__") = VERSION;  // Version is set by CMAKE
}
