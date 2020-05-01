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

#ifdef OPTION_VCELL
    er = simInitAndLoad(fileroot.c_str(), filename.c_str(), &pSim_, flags.c_str(),
        new SimpleValueProviderFactory(), new SimpleMesh());
#else
    er =
        simInitAndLoad(fileroot.c_str(), filename.c_str(), &pSim_, flags.c_str());
#endif
    if(!er) {
        // if (!tflag && pSim_->graphss && pSim_->graphss->graphics != 0)
        // gl2glutInit(0, "");
        er = simUpdateAndDisplay(pSim_);
    }
    if(!er)
        er = scmdopenfiles((cmdssptr)pSim_->cmds, wflag);
    if(er) {
        simLog(pSim_, 4, "%sSimulation skipped\n", er ? "\n" : "");
    }
    else {
        fflush(stdout);
        fflush(stderr);
        if(!pSim_->graphss || pSim_->graphss->graphics == 0) {
            er = smolsimulate(pSim_);
            endsimulate(pSim_, er);
        }
        else {
            smolsimulategl(pSim_);
        }
    }
    simfree(pSim_);
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

    /* Main function  */
    m.def("getDim", &getDim);
    m.def("setDim", &setDim);
    m.def("getRandomSeed", &getRandomSeed);
    m.def("setRandomSeed", &setRandomSeed);
    m.def("getBoundaries", &getBounds);
    m.def("setBoundaries", &setBounds);

    /* box/molperbox etc */
    m.def("setPartitions", &setPartitions);
    /* Molecules */
    m.def("addSpecies", &addSpecies, "name"_a, "mollist"_a = "");
    m.def("setSpeciesStyle", &setSpeciesStyle);
    m.def("setSpeciesMobility", &setSpeciesMobility, "species"_a,
        "state"_a, "diffConst"_a, "drift"_a = std::vector<double>(),
        "difmatrix"_a = std::vector<double>());
    /* Surface */
    m.def("addSurface", &addSurface, "name"_a);
    m.def("setSurfaceAction", &setSurfaceAction);
    m.def("addSurfaceMolecules", &addSurfaceMolecules);
    /* Panel */
    m.def("addPanel", &addPanel);
    /* Compartment */
    m.def("addCompartment", &addCompartment);
    m.def("addCompartmentSurface", &addCompartmentSurface);
    m.def("addCompartmentPoint", &addCompartmentPoint);
    m.def("addComparmentMolecules", &addCompartmentMolecules);
    /* Reaction */
    m.def("addReaction", &addReaction, "reac"_a, "reactant1"_a,
        "rstate1"_a, "reactant2"_a, "rstate2"_a, "products"_a,
        "productstates"_a, "rate"_a);
    m.def("setReactionRegion", &setReactionRegion);
    m.def("setBoundaryType", &setBoundaryType);
    m.def("addMolList", &addMolList);
    /* data */
    m.def("getMoleculeCount", &getMoleculeCount);
    /* Graphics */
    m.def("setGraphicsParams", &setGraphicsParams);
    /* Simulation */
    m.def("setTimes", &setSimTimes);
    m.def("updateSim", &updateSim);
    m.def("runSim", &runSim, "stoptime"_a, "dt"_a, "display"_a = true);
    m.def("runUntil", &runUntil, "breaktime"_a, "dt"_a = 0.0,
        "display"_a = true);
    m.def("addSolutionMolecules", &addSolutionMolecules);
    m.def("getDt", &getDt);
    m.def("setDt", &setDt);

    /* Function */
    m.def("load_model", &init_and_run, "filepath"_a, "args"_a = "",
        "Load model from a txt file");

    /* attributes */
    m.attr("__version__") = VERSION;  // Version is set by CMAKE
}
