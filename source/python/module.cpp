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

#include "../Smoldyn/smoldynfuncs.h"

#include "Simulation.h"
#include "SmoldynSpecies.h"

using namespace pybind11::literals;  // for _a

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

    py::class_<SmoldynDefine>(m, "__Define__")
        .def(py::init<>())
        .def("__setitem__", &SmoldynDefine::setDefine)
        .def("__getitem__", &SmoldynDefine::getDefine)
        .def("__repr__", &SmoldynDefine::__repr__);

    /* Species */
    py::class_<SmoldynSpecies>(m, "Species")
        .def(py::init<const string&>())
        .def("__repr__",
            [](const SmoldynSpecies& sp) {
                return "<smoldyn.Species: name=" + sp.getName() + ">";
            })
        .def_property("difc", &SmoldynSpecies::getDiffConst,
            &SmoldynSpecies::setDiffConst)
        .def_property(
            "color", &SmoldynSpecies::getColor, &SmoldynSpecies::setColor)
        .def_property("display_size", &SmoldynSpecies::getDisplaySize,
            &SmoldynSpecies::setDisplaySize);

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

    /* Simulation class */
    py::class_<Simulation>(m, "Simulation")
        .def(py::init<bool>(), "debug"_a = false)
        .def_property_readonly("define", &Simulation::getDefine,
            py::return_value_policy::reference)
        .def_property("dim", &Simulation::getDim, &Simulation::setDim)
        .def_property(
            "seed", &Simulation::getRandomSeed, &Simulation::setRandomSeed)
        .def_property("bounds", &Simulation::getBounds, &Simulation::setBounds)
        .def("run", &Simulation::run, "stoptime"_a, "starttime"_a = 0.0,
            "dt"_a = 1e-5, "display"_a = true)
        .def("setPartitions", &Simulation::setPartitions)
        .def("addSpecies", &Simulation::addSpecies, "name"_a, "mollist"_a = "")
        .def("setSpeciesMobility", &Simulation::setSpeciesMobility, "species"_a,
            "state"_a, "diffConst"_a, "drift"_a = std::vector<double>(),
            "difmatrix"_a = std::vector<double>())
        .def("addSurface", &Simulation::addSurface, "name"_a)
        .def("setSurfaceAction", &Simulation::setSurfaceAction)
        .def("addSurfaceMolecules", &Simulation::addSurfaceMolecules)
        .def("addPanel", &Simulation::addPanel)
        .def("addCompartment", &Simulation::addCompartment)
        .def("addCompartmentSurface", &Simulation::addCompartmentSurface)
        .def("addCompartmentPoint", &Simulation::addCompartmentPoint)
        .def("addComparmentMolecules", &Simulation::addCompartmentMolecules)
        .def("addReaction", &Simulation::addReaction, "reac"_a, "reactant1"_a,
            "rstate1"_a, "reactant2"_a, "rstate2"_a, "products"_a,
            "productstates"_a, "rate"_a)
        .def("setReactionRegion", &Simulation::setReactionRegion)
        .def("setBoundaryType", &Simulation::setBoundaryType)
        .def("addMolList", &Simulation::addMolList)
        /* Graphics */
        .def("setGraphicsParams", &Simulation::setGraphicsParams)
        .def("setMoleculeStyle", &Simulation::setMoleculeStyle)
        /* Simulation */
        .def("setSimTimes", &Simulation::setSimTimes)
        .def("runSim",
            [](const Simulation& s) { return smolRunSim(s.simPtr()); })
        .def(
            "update", [](const Simulation& s) { return simupdate(s.simPtr()); })
        .def("runSimUntil",
            [](const Simulation& s, double breaktime) {
                return smolRunSimUntil(s.simPtr(), breaktime);
            })
        .def("addSolutionMolecules", &Simulation::addSolutionMolecules)
        .def("displaySim",
            [](const Simulation& s) { return smolDisplaySim(s.simPtr()); });

    /* Function */
    m.def("load_model", &init_and_run, "Load model from a txt file");

    /* attributes */
    m.attr("__version__") = SMOLDYN_VERSION;
}
