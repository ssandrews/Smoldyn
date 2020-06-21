/*
 * Python module.
 * Author: Dilawar Singh <dilawar.s.rajput@gmail.com>
 */

#include <iostream>
#include <vector>
#include <string>
#include <map>

using namespace std;

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"
#include "pybind11/functional.h"

// Globals are also declarated here.

#include "../lib/SimCommand.h"
#include "../Smoldyn/smoldynfuncs.h"
#include "Smoldyn.h"

using namespace pybind11::literals;  // for _a

// Defined in SimCommand.c file.
extern int scmdopenfiles(cmdssptr cmds, int overwrite);

extern vector<vector<double>> &getData();

/* --------------------------------------------------------------------------*/
/** @Synopsis  Initialize and run the simulation from given file.
 *
 * This function takes input file and run the simulation. This is more or less
 * same as `main()` function in smoldyn.c file.
 *
 * @Param filepath @Param flags
 *
 */
/* ----------------------------------------------------------------------------*/
int init_and_run(const string &filepath, const string &flags)
{
    int  er = 0, wflag = 0;
    auto p = splitPath(filepath);

    // DO NOT USE global simptr
    simptr psim;

#ifdef OPTION_VCELL
    er = simInitAndLoad(p.first.c_str(), p.second.c_str(), &psim, flags.c_str(),
        new SimpleValueProviderFactory(), new SimpleMesh());
#else
    er = simInitAndLoad(p.first.c_str(), p.second.c_str(), &psim, flags.c_str());
#endif
    if(!er) {
        // if (!tflag && psim->graphss && psim->graphss->graphics != 0)
        // gl2glutInit(0, "");
        er = simUpdateAndDisplay(psim);
    }
    if(!er)
        er = scmdopenfiles((cmdssptr)psim->cmds, wflag);
    if(er) {
        simLog(psim, 4, "%sSimulation skipped\n", er ? "\n" : "");
    }
    else {
        fflush(stdout);
        fflush(stderr);
        if(!psim->graphss || psim->graphss->graphics == 0) {
            er = smolsimulate(psim);
            endsimulate(psim, er);
        }
        else {
            smolsimulategl(psim);
        }
    }
    simfree(psim);
    simfuncfree();
    return er;
}

PYBIND11_MODULE(_smoldyn, m)
{
    // py::options options;
    // options.disable_function_signatures();

    m.doc() = R"pbdoc(
        Python interface of smoldyn simulator.
    )pbdoc";

    py::enum_<SrfAction>(m, "SrfAction")
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

    py::enum_<MolecState>(m, "MolecState")
        .value("soln", MolecState::MSsoln)
        .value("front", MolecState::MSfront)
        .value("back", MolecState::MSback)
        .value("up", MolecState::MSup)
        .value("down", MolecState::MSdown)
        .value("bsoln", MolecState::MSbsoln)
        .value("all", MolecState::MSall)
        .value("none", MolecState::MSnone)
        .value("some", MolecState::MSsome);

    py::enum_<PanelFace>(m, "PanelFace")
        .value("front", PanelFace::PFfront)
        .value("back", PanelFace::PFback)
        .value("none", PanelFace::PFnone)
        .value("both", PanelFace::PFboth);

    py::enum_<PanelShape>(m, "PanelShape")
        .value("rect", PanelShape::PSrect)
        .value("tri", PanelShape::PStri)
        .value("sph", PanelShape::PSsph)
        .value("cyl", PanelShape::PScyl)
        .value("hemi", PanelShape::PShemi)
        .value("disk", PanelShape::PSdisk)
        .value("all", PanelShape::PSall)
        .value("none", PanelShape::PSnone);

    /* Error codes.  */
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

    /* simptr */
    // TODO: Only simple members are exposed (readonly).
    py::class_<simstruct>(m, "simstruct")
        //.def(py::init<>())
        .def_readonly("condition", &simstruct::condition, "Structure condition")
        .def_readonly("logfile", &simstruct::logfile, "configuration file path")
        .def_readonly("filepath", &simstruct::filepath, "configuration file path")
        .def_readonly("filename", &simstruct::filename, "configuration file name")
        .def_readonly("flags", &simstruct::flags, "command-line options from user")
        .def_readonly(
            "clockstt", &simstruct::clockstt, "clock starting time of simulation")
        .def_readonly(
            "elapsedtime", &simstruct::elapsedtime, "elapsed time of simulation")
        .def_readonly("randseed", &simstruct::randseed, "random number generator seed")
        .def_readonly(
            "eventcount", &simstruct::eventcount, "counter for simulation events")
        .def_readonly("maxvar", &simstruct::maxvar, "allocated user-settable variables")
        .def_readonly("nvar", &simstruct::nvar, "number of user-settable variables")
        .def_readonly("dim", &simstruct::dim, " dimensionality of space.")
        .def_readonly("accur", &simstruct::accur, " accuracy, on scale from 0 to 10")
        .def_readonly("time", &simstruct::time, " current time in simulation")
        .def_readonly("tmin", &simstruct::tmin, " simulation start time")
        .def_readonly("tmax", &simstruct::tmax, " simulation end time")
        .def_readonly("tbreak", &simstruct::tbreak, " simulation break time")
        .def_readonly("dt", &simstruct::dt, " simulation time step")
        .def_readonly("quitatend", &simstruct::quitatend, " simulation quits at the end")
        .def_readonly("ruless", &simstruct::ruless, " rule superstructure")
        .def_readonly("mols", &simstruct::mols, " molecule superstructure")
        .def_readonly("srfss", &simstruct::srfss, " surface superstructure")
        .def_readonly("boxs", &simstruct::boxs, " box superstructure")
        .def_readonly("cmptss", &simstruct::cmptss, " compartment superstructure")
        .def_readonly("portss", &simstruct::portss, " port superstructure")
        .def_readonly("latticess", &simstruct::latticess, " lattice superstructure")
        .def_readonly("bngss", &simstruct::bngss, " bionetget superstructure")
        .def_readonly("filss", &simstruct::filss, " filament superstructure")
        .def_readonly("cmds", &simstruct::cmds, " command superstructure")
        .def_readonly("graphss", &simstruct::graphss, " graphics superstructure")
        ;

    /*******************
     *  Miscellaneous  *
     *******************/
    m.def("getVersion", &smolGetVersion);

    /************
     *  Errors  *
     ************/
    m.def("setDebugMode", &smolSetDebugMode);
    m.def("errorCodeToString", &smolErrorCodeToString);

    /************************
     *  Sim struture (use with care).
     ************************/
    m.def(
        "newSim",
        [](int dim, vector<double> &lowbounds, vector<double> &highbounds) {
            auto sim = smolNewSim(dim, &lowbounds[0], &highbounds[0]);
            simptrs_.push_back(sim);
            return sim;
        },
        py::return_value_policy::reference);

    // make a simptr current working simptr. The simptr must be initialized
    // properly. 
    // FIXME: Tests are not exhaustive.
    m.def("setCurSimStruct", [](simstruct* const ptr) {  cursim_ = ptr; }, "change current simstruct");
    m.def("getCurSimStruct", []() { return cursim_;}, "Get current sim struct");

    m.def("updateSim", [](void) { return smolUpdateSim(cursim_); });
    m.def("runTimeStep", [](void) { return smolRunTimeStep(cursim_); });
    m.def("runSim", [](void) { return smolRunSim(cursim_); });
    m.def("runSimUntil",
        [](double breaktime) { return smolRunSimUntil(cursim_, breaktime); });
    m.def("freeSim", [](void) { return smolFreeSim(cursim_); });
    m.def("displaySim", [](void) { return smolDisplaySim(cursim_); });

    // Extra (not in C api).
    m.def("run", &run, "stoptime"_a, "dt"_a, "display"_a = true);
    m.def("runUntil", &runUntil, "breaktime"_a, "dt"_a = 0.0, "display"_a = true);

    /*****************************
     *  Read configuration file  *
     *****************************/
    m.def("prepareSimFromFile", [](const char *filepath, const char *flags) {
        auto path = splitPath(string(filepath));
        return smolPrepareSimFromFile(path.first.c_str(), path.second.c_str(), flags);
    });
    m.def("loadSimFromFile", [](const string &filepath, const char *flags) -> ErrorCode {
        auto   p    = splitPath(filepath);
        simptr psim = cursim_;
        assert(psim);
        return smolLoadSimFromFile(p.first.c_str(), p.second.c_str(), &psim, flags);
    });
    m.def("readConfigString", [](const char *statement, char *params) -> ErrorCode {
        return smolReadConfigString(cursim_, statement, params);
    });

    /*************************
     *  Simulation settings  *
     *************************/
    // enum ErrorCode smolSetSimTimes(
    //         simptr sim, double timestart, double timestop, double timestep);
    m.def("setSimTimes",
        [](double timestart, double timestop, double timestep) -> ErrorCode {
            return smolSetSimTimes(cursim_, timestart, timestop, timestep);
        });

    // enum ErrorCode smolSetTimeStart(simptr sim, double timestart);
    m.def("setTimeStart",
        [](double time) -> ErrorCode { return smolSetTimeStart(cursim_, time); });
    m.def("getTimeStart", []() { return cursim_->tmin; });

    // enum ErrorCode smolSetTimeStop(simptr sim, double timestop);
    m.def("setTimeStop",
        [](double timestop) -> ErrorCode { return smolSetTimeStop(cursim_, timestop); });
    m.def("getTimeStop", []() { return cursim_->tmax; });

    // enum ErrorCode smolSetTimeNow(simptr sim, double timenow);
    m.def("setTimeNow",
        [](double timenow) -> ErrorCode { return smolSetTimeNow(cursim_, timenow); });

    // enum ErrorCode smolSetTimeStep(simptr sim, double timestep);
    m.def("setTimeStep",
        [](double timestep) -> ErrorCode { return smolSetTimeStep(cursim_, timestep); });
    m.def("getTimeStep", []() { return cursim_->dt; });

    // enum ErrorCode smolSetRandomSeed(simptr sim, long int seed);
    m.def("setRandomSeed",
        [](long int seed) -> ErrorCode { return smolSetRandomSeed(cursim_, seed); });

    // enum ErrorCode smolSetPartitions(simptr sim, const char *method, double
    // value);
    m.def("setPartitions", [](const char *method, double value) {
        return smolSetPartitions(cursim_, method, value);
    });

    /*********************************
     *  Graphics related functions.  *
     *********************************/
    // enum ErrorCode smolSetGraphicsParams(simptr sim, const char *method, int
    // timesteps, int delay);
    m.def("setGraphicsParams",
        [](const char *method, int timestep, int delay) -> ErrorCode {
            return smolSetGraphicsParams(cursim_, method, timestep, delay);
        });

    // enum ErrorCode smolSetTiffParams(simptr sim, int timesteps,
    //     const char *tiffname, int lowcount, int highcount);
    m.def("setTiffParams",
        [](int timesteps, const char *tiffname, int lowcount,
            int highcount) -> ErrorCode {
            return smolSetTiffParams(cursim_, timesteps, tiffname, lowcount, highcount);
        });

    // enum ErrorCode smolSetLightParams(simptr sim, int lightindex, double
    // *ambient,
    //     double *diffuse, double *specular, double *position);
    m.def("setLightParams",
        [](int lightindex, vector<double> &ambient, vector<double> &diffuse,
            vector<double> &specular, vector<double> &position) -> ErrorCode {
            return smolSetLightParams(cursim_, lightindex, &ambient[0], &diffuse[0],
                &specular[0], &position[0]);
        });

    // enum ErrorCode smolSetBackgroundStyle(simptr sim, double *color);
    m.def("setBackgroundStyle", [](char *color) -> ErrorCode {
        array<double, 4> rgba = {0, 0, 0, 1.0};
        graphicsreadcolor(&color, &rgba[0]);
        return smolSetBackgroundStyle(cursim_, &rgba[0]);
    });

    // enum ErrorCode smolSetFrameStyle(simptr sim, double thickness, double
    // *color);
    m.def("setFrameStyle", [](double thickness, char *color) -> ErrorCode {
        array<double, 4> rgba = {0, 0, 0, 1.0};
        graphicsreadcolor(&color, &rgba[0]);
        return smolSetBackgroundStyle(cursim_, &rgba[0]);
    });

    // enum ErrorCode smolSetGridStyle(simptr sim, double thickness, double
    // *color);
    m.def("setGridStyle", [](double thickness, char *color) {
        array<double, 4> rgba = {0, 0, 0, 1.0};
        graphicsreadcolor(&color, &rgba[0]);
        return smolSetGridStyle(cursim_, thickness, &rgba[0]);
    });

    // enum ErrorCode smolSetTextStyle(simptr sim, double *color);
    m.def("setTextStyle", [](char *color) -> ErrorCode {
        array<double, 4> rgba = {0, 0, 0, 1.0};
        graphicsreadcolor(&color, &rgba[0]);
        return smolSetTextStyle(cursim_, &rgba[0]);
    });

    // enum ErrorCode smolAddTextDisplay(simptr sim, char *item);
    m.def("addTextDisplay",
        [](char *item) -> ErrorCode { return smolAddTextDisplay(cursim_, item); });

    /***********************
     *  Runtime commands.  *
     ***********************/
    // enum ErrorCode smolSetOutputPath(simptr sim, const char *path);
    m.def("setOutputPath",
        [](const char *path) { return smolSetOutputPath(cursim_, path); });

    // enum ErrorCode smolAddOutputFile(simptr sim, char *filename, int suffix,
    // int append);
    m.def("addOutputFile", [](char *filename, bool suffix, bool append) {
        return smolAddOutputFile(cursim_, filename, suffix, append);
    });

    // enum ErrorCode smolAddCommand(simptr sim, char type, double on, double
    // off,
    //     double step, double multiplier, const char *commandstring);
    m.def("addCommand", [](char type, double on, double off, double step,
                            double multiplier, const char *commandstring) {
        return smolAddCommand(cursim_, type, on, off, step, multiplier, commandstring);
    });

    // enum ErrorCode smolAddCommandFromString(simptr sim, char *string);
    m.def("addCommmandFromString",
        [](char *command) { return smolAddCommandFromString(cursim_, command); });

    /***************
     *  Molecules  *
     ***************/
    // enum ErrorCode smolAddSpecies(simptr sim, const char *species, const char
    // *mollist);
    m.def(
        "addSpecies",
        [](const char *species, const char *mollist) {
            return smolAddSpecies(cursim_, species, mollist);
        },
        "species"_a, "mollist"_a = "");

    // int   smolGetSpeciesIndex(simptr sim, const char *species);
    m.def("getSpeciesIndex",
        [](const char *species) -> int { return smolGetSpeciesIndex(cursim_, species); });

    // int   smolGetSpeciesIndexNT(simptr sim, const char *species);
    m.def("speciesIndexNT",
        [](const char *species) { return smolGetSpeciesIndexNT(cursim_, species); });

    // char *smolGetSpeciesName(simptr sim, int speciesindex, char *species);
    m.def("getSpeciesName", [](int speciesindex, char *species) {
        return smolGetSpeciesName(cursim_, speciesindex, species);
    });

    // enum ErrorCode smolSetSpeciesMobility(simptr sim, const char *species,
    //     enum MolecState state, double difc, double *drift, double
    //     *difmatrix);
    m.def(
        "setSpeciesMobility",
        [](const char *species, MolecState state, double difc, vector<double> &drift,
            vector<double> &difmatrix) {
            return smolSetSpeciesMobility(
                cursim_, species, state, difc, &drift[0], &difmatrix[0]);
        },
        "species"_a, "state"_a, "diffConst"_a, "drift"_a = std::vector<double>(),
        "difmatrix"_a = std::vector<double>());

    //?? needs function smolSetSpeciesSurfaceDrift
    // enum ErrorCode smolAddMolList(simptr sim, const char *mollist);
    m.def("addMolList",
        [](const char *mollist) { return smolAddMolList(cursim_, mollist); });

    // int   smolGetMolListIndex(simptr sim, const char *mollist);
    m.def("getMolListIndex",
        [](const char *mollist) { return smolGetMolListIndex(cursim_, mollist); });

    // int   smolGetMolListIndexNT(simptr sim, const char *mollist);
    m.def("molListIndexNT",
        [](const char *mollist) { return smolGetMolListIndexNT(cursim_, mollist); });

    // char *smolGetMolListName(simptr sim, int mollistindex, char *mollist);
    m.def("getMolListName", [](int mollistindex, char *species) {
        return smolGetMolListName(cursim_, mollistindex, species);
    });

    // enum ErrorCode smolSetMolList(simptr sim, const char *species, enum
    // MolecState state, const char *mollist);
    m.def("setMolList", [](const char *species, MolecState state, const char *mollist) {
        return smolSetMolList(cursim_, species, state, mollist);
    });

    // enum ErrorCode smolSetMaxMolecules(simptr sim, int maxmolecules);
    m.def("setMaxMolecules",
        [](int maxmolecules) { return smolSetMaxMolecules(cursim_, maxmolecules); });

    // enum ErrorCode smolAddSolutionMolecules(simptr sim, const char *species,
    //     int number, double *lowposition, double *highposition);
    m.def("addSolutionMolecules",
        [](const char *species, int number, vector<double> &lowposition,
            vector<double> &highposition) {
            return smolAddSolutionMolecules(
                cursim_, species, number, &lowposition[0], &highposition[0]);
        });

    // enum ErrorCode smolAddCompartmentMolecules(
    //     simptr sim, const char *species, int number, const char
    //     *compartment);
    m.def("addCompartmentMolecules",
        [](const char *species, int number, const char *compartment) {
            return smolAddCompartmentMolecules(cursim_, species, number, compartment);
        });

    // enum ErrorCode smolAddSurfaceMolecules(simptr sim, const char *species,
    //     enum MolecState state, int number, const char *surface,
    //     enum PanelShape panelshape, const char *panel, double *position);
    m.def("addSurfaceMolecules",
        [](const char *species, MolecState state, int number, const char *surface,
            PanelShape panelshape, const char *panel,
            vector<double> &position) -> ErrorCode {
            return smolAddSurfaceMolecules(cursim_, species, state, number, surface,
                panelshape, panel, &position[0]);
        });

    // int smolGetMoleculeCount(simptr sim, const char *species, enum MolecState
    // state);
    m.def("getMoleculeCount", [](const char *species, MolecState state) {
        return smolGetMoleculeCount(cursim_, species, state);
    });

    // enum ErrorCode smolSetMoleculeStyle(simptr sim, const char *species,
    //     enum MolecState state, double size, double *color);
    m.def("setMoleculeStyle",
        [](const char *species, MolecState state, double size, char *color) {
            auto rgba = color2RGBA(color);
            return smolSetMoleculeStyle(cursim_, species, state, size, &rgba[0]);
        });

    /*************
     *  Surface  *
     *************/
    // enum ErrorCode smolSetBoundaryType(simptr sim, int dimension, int
    // highside, char type);
    m.def("setBoundaryType", [](int dimension, int highside, char type) {
        return smolSetBoundaryType(cursim_, dimension, highside, type);
    });

    // enum ErrorCode smolAddSurface(simptr sim, const char *surface);
    m.def("addSurface",
        [](const char *surface) { return smolAddSurface(cursim_, surface); });

    // int molGetSurfaceIndex(simptr sim, const char *surface);
    m.def("getSurfaceIndex",
        [](const char *surface) { return smolGetSurfaceIndex(cursim_, surface); });

    // int            smolGetSurfaceIndexNT(simptr sim, const char *surface);
    m.def("getSurfaceIndexNT",
        [](const char *surface) { return smolGetSurfaceIndexNT(cursim_, surface); });

    // char *smolGetSurfaceName(simptr sim, int surfaceindex, char *surface);
    m.def("getSurfaceName", [](int surfaceindex, char *surface) {
        return smolGetSurfaceName(cursim_, surfaceindex, surface);
    });

    // enum ErrorCode smolSetSurfaceAction(simptr sim, const char *surface,
    //     enum PanelFace face, const char *species, enum MolecState state,
    //     enum SrfAction action);
    m.def("setSurfaceAction", [](const char *surface, PanelFace face, const char *species,
                                  MolecState state, SrfAction action) {
        return smolSetSurfaceAction(cursim_, surface, face, species, state, action);
    });

    // enum ErrorCode smolSetSurfaceRate(simptr sim, const char *surface,
    //     const char *species, enum MolecState state, enum MolecState state1,
    //     enum MolecState state2, double rate, const char *newspecies,
    //     int isinternal);
    m.def("setSurfaceRate",
        [](const char *surface, const char *species, MolecState state, MolecState state1,
            MolecState state2, double rate, const char *newspecies, bool isinternal) {
            return smolSetSurfaceRate(cursim_, surface, species, state, state1, state2,
                rate, newspecies, isinternal);
        });
    // enum ErrorCode smolAddPanel(simptr sim, const char *surface,
    //     enum PanelShape panelshape, const char *panel, const char
    //     *axisstring, double *params);
    m.def("addPanel", [](const char *surface, PanelShape panelshape, const char *panel,
                          const char *axisstring, vector<double> &params) {
        return smolAddPanel(cursim_, surface, panelshape, panel, axisstring, &params[0]);
    });
    // int            smolGetPanelIndex(simptr sim, const char *surface,
    //                enum PanelShape *panelshapeptr, const char *panel);
    m.def("getPanelIndex",
        [](const char *surface, PanelShape *panelshape, const char *panel) {
            return smolGetPanelIndex(cursim_, surface, panelshape, panel);
        });

    // int            smolGetPanelIndexNT(simptr sim, const char *surface,
    //                enum PanelShape *panelshapeptr, const char *panel);
    m.def("getPanelIndexNT",
        [](const char *surface, PanelShape *panelshape, const char *panel) {
            return smolGetPanelIndexNT(cursim_, surface, panelshape, panel);
        });

    // char*  smolGetPanelName(simptr sim, const char *surface, enum PanelShape
    // panelshape, int panelindex, char *panel);
    m.def("getPanelName",
        [](const char *surface, PanelShape panelshape, int panelindex, char *panel) {
            return smolGetPanelName(cursim_, surface, panelshape, panelindex, panel);
        });

    // enum ErrorCode smolSetPanelJump(simptr sim, const char *surface,
    //     const char *panel1, enum PanelFace face1, const char *panel2,
    //     enum PanelFace face2, int isbidirectional);
    m.def("setPanelJump", [](const char *surface, const char *panel1, PanelFace face1,
                              const char *panel2, PanelFace face2, bool isbidirectional) {
        return smolSetPanelJump(
            cursim_, surface, panel1, face1, panel2, face2, isbidirectional);
    });

    // enum ErrorCode smolAddSurfaceUnboundedEmitter(simptr sim,
    //     const char *surface, enum PanelFace face, const char *species,
    //     double emitamount, double *emitposition);
    m.def("addSurfaceUnboundedEmitter",
        [](const char *surface, PanelFace face, const char *species, double emitamount,
            vector<double> &emitposition) {
            return smolAddSurfaceUnboundedEmitter(
                cursim_, surface, face, species, emitamount, &emitposition[0]);
        });

    // enum ErrorCode smolSetSurfaceSimParams(
    //     simptr sim, const char *parameter, double value);
    m.def("setSurfaceSimParams", [](const char *parameter, double value) {
        return smolSetSurfaceSimParams(cursim_, parameter, value);
    });

    // enum ErrorCode smolAddPanelNeighbor(simptr sim, const char *surface1,
    //     const char *panel1, const char *surface2, const char *panel2,
    //     int reciprocal);
    m.def("addPanelNeighbhor",
        [](const char *surface1, const char *panel1, const char *surface2,
            const char *panel2, int reciprocal) {
            return smolAddPanelNeighbor(
                cursim_, surface1, panel1, surface2, panel2, reciprocal);
        });

    // enum ErrorCode smolSetSurfaceStyle(simptr sim, const char *surface,
    //     enum PanelFace face, enum DrawMode mode, double thickness,
    //     double *color, int stipplefactor, int stipplepattern, double
    //     shininess);
    m.def("setSurfaceStyle",
        [](const char *surface, PanelFace face, DrawMode mode, double thickness,
            char *color, int stipplefactor, int stipplepattern, double shininess) {
            array<double, 4> rgba = {0, 0, 0, 1.0};
            graphicsreadcolor(&color, &rgba[0]);
            return smolSetSurfaceStyle(cursim_, surface, face, mode, thickness, &rgba[0],
                stipplefactor, stipplepattern, shininess);
        });

    /*****************
     *  Compartment  *
     *****************/
    // enum ErrorCode smolAddCompartment(simptr sim, const char *compartment);
    m.def("addCompartment",
        [](const char *compartment) { return smolAddCompartment(cursim_, compartment); });

    // int smolGetCompartmentIndex(simptr sim, const char *compartment);
    m.def("getCompartmentIndex", [](const char *compartment) {
        return smolGetCompartmentIndex(cursim_, compartment);
    });

    // int smolGetCompartmentIndexNT(simptr sim, const char *compartment);
    m.def("getCompartmentIndexNT", [](const char *compartment) {
        return smolGetCompartmentIndexNT(cursim_, compartment);
    });
    // char * smolGetCompartmentName(simptr sim, int compartmentindex, char
    // *compartment);
    m.def("getCompartmentName", [](int compartmentindex, char *compartment) {
        return smolGetCompartmentName(cursim_, compartmentindex, compartment);
    });

    // enum ErrorCode smolAddCompartmentSurface(simptr sim, const char
    // *compartment, const char *surface);
    m.def("addCompartmentSurface", [](const char *compartment, const char *surface) {
        return smolAddCompartmentSurface(cursim_, compartment, surface);
    });

    // enum ErrorCode smolAddCompartmentPoint(simptr sim, const char
    // *compartment, double *point);
    m.def("addCompartmentPoint", [](const char *compartment, vector<double> &point) {
        return smolAddCompartmentPoint(cursim_, compartment, &point[0]);
    });

    // enum ErrorCode smolAddCompartmentLogic(simptr sim, const char
    // *compartment, enum CmptLogic logic, const char *compartment2);
    m.def("addCompartmentLogic",
        [](const char *compartment, CmptLogic logic, const char *compartment2) {
            return smolAddCompartmentLogic(cursim_, compartment, logic, compartment2);
        });

    /***************
     *  Reactions  *
     ***************/
    // enum ErrorCode smolAddReaction(simptr sim, const char *reaction,
    //         const char *reactant1, enum MolecState rstate1, const char
    //         *reactant2, enum MolecState rstate2, int nproduct, const char
    //         **productspecies, enum MolecState *productstates, double rate);
    m.def("addReaction",
        [](const char *         reaction,           // Name of the reaction.
            const char *        reactant1,          // First reactant
            MolecState          rstate1,            // First reactant state
            const char *        reactant2,          // Second reactant.
            MolecState          rstate2,            // second reactant state.
            vector<string> &    productSpeciesStr,  // product species.
            vector<MolecState> &productStates,      // product state.
            double              rate                // rate
        ) {
            // NOTE: Can't use vector<const char*> in the function argument.
            // We'll runinto wchar_t vs char* issue from python2/python3
            // str/unicode fiasco. Be safe, use string and cast to const char*
            // by ourselves.

            size_t nprd = productStates.size();

            vector<const char *> productSpecies(nprd);
            for(size_t i = 0; i < nprd; i++)
                productSpecies[i] = productSpeciesStr[i].c_str();

            // cout << reaction << " r1: '" << reactant1 << "' r2: '" <<
            // reactant2
            //      << "' " << " rate=" << rate;
            // for(auto s : productSpeciesStr)
            //     cout << " '" << s << "'";
            // cout << endl;

            // Not sure if this is needed here.
            // if(reactant2 == "")
            // reactant2[0] = '\0';

            return smolAddReaction(cursim_, reaction, reactant1, rstate1, reactant2,
                rstate2, productSpecies.size(), productSpecies.data(), &productStates[0],
                rate);
        });

    // int smolGetReactionIndex(simptr sim, int *orderptr, const char
    // *reaction);
    m.def("getReactionIndex", [](vector<int> &order, const char *reaction) {
        return smolGetReactionIndex(cursim_, &order[0], reaction);
    });

    // int smolGetReactionIndexNT(simptr sim, int *orderptr, const char
    // *reaction);
    m.def("getReactionIndexNT", [](vector<int> &order, const char *reaction) {
        return smolGetReactionIndexNT(cursim_, &order[0], reaction);
    });

    // char *smolGetReactionName(simptr sim, int order, int reactionindex, char
    // *reaction);
    m.def("getReactionName", [](int order, int reactionindex, char *reaction) {
        return smolGetReactionName(cursim_, order, reactionindex, reaction);
    });

    // enum ErrorCode smolSetReactionRate(simptr sim, const char *reaction,
    // double rate, int type);
    m.def("setReactionRate", [](const char *reaction, double rate, int type) {
        return smolSetReactionRate(cursim_, reaction, rate, type);
    });

    // enum ErrorCode smolSetReactionRegion(simptr sim, const char *reaction,
    // const char *compartment, const char *surface);
    m.def("setReactionRegion",
        [](const char *reaction, const char *compartment, const char *surface) {
            return smolSetReactionRegion(cursim_, reaction, compartment, surface);
        });

    // enum ErrorCode smolSetReactionProducts(simptr sim, const char *reaction,
    //     enum RevParam method, double parameter, const char *product,
    //     double *position);
    m.def(
        "setReactionProducts", [](const char *reaction, RevParam method, double parameter,
                                   const char *product, vector<double> &position) {
            return smolSetReactionProducts(
                cursim_, reaction, method, parameter, product, &position[0]);
        });

    /***********
     *  Ports  *
     ***********/
    // enum ErrorCode smolAddPort(simptr sim, const char *port, const char
    // *surface, enum PanelFace face);
    m.def("addPort", [](const char *port, const char *surface, PanelFace face) {
        return smolAddPort(cursim_, port, surface, face);
    });

    // int smolGetPortIndex(simptr sim, const char *port);
    m.def(
        "getPortIndex", [](const char *port) { return smolGetPortIndex(cursim_, port); });

    // int smolGetPortIndexNT(simptr sim, const char *port);
    m.def("getPortIndexNT",
        [](const char *port) { return smolGetPortIndexNT(cursim_, port); });

    // char * smolGetPortName(simptr sim, int portindex, char *port);
    m.def("getPortName", [](int portindex, char *port) {
        return smolGetPortName(cursim_, portindex, port);
    });

    // enum ErrorCode smolAddPortMolecules(simptr sim, const char *port,
    //     int nmolec, const char *species, double **positions);
    m.def("addPortMolecule", [](const char *port, int nmolec, const char *species,
                                 vector<vector<double>> &pos) {
        std::vector<double *> ptrs;
        for(auto &vec : pos)
            ptrs.push_back(vec.data());
        return smolAddPortMolecules(cursim_, port, nmolec, species, ptrs.data());
    });

    // int smolGetPortMolecules(simptr sim, const char *port, const char
    // *species, enum MolecState state, int remove);
    m.def("getPortMolecules",
        [](const char *port, const char *species, MolecState state, bool remove) {
            return smolGetPortMolecules(cursim_, port, species, state, remove);
        });

    /**************
     *  Lattices  *
     **************/
    // enum ErrorCode smolAddLattice(simptr sim, const char *lattice,
    //         const double *min, const double *max, const double *dx, const
    //         char *btype);
    m.def("addLattice",
        [](const char *lattice, const vector<double> &min, const std::vector<double> &max,
            const vector<double> dx, const char *btype) {
            return smolAddLattice(cursim_, lattice, &min[0], &max[0], &dx[0], btype);
        });

    // enum ErrorCode smolAddLatticePort(simptr sim, const char *lattice, const
    // char *port);
    m.def("addLatticePort", [](const char *lattice, const char *port) {
        return smolAddLatticePort(cursim_, lattice, port);
    });

    // enum ErrorCode smolAddLatticeSpecies(simptr sim, const char *lattice,
    // const char *species);
    m.def("addLatticeSpecies", [](const char *lattice, const char *species) {
        return smolAddLatticeSpecies(cursim_, lattice, species);
    });

    // int   smolGetLatticeIndex(simptr sim, const char *lattice);
    m.def("getLatticeIndex",
        [](const char *lattice) { return smolGetLatticeIndex(cursim_, lattice); });

    // int   smolGetLatticeIndexNT(simptr sim, const char *lattice);
    m.def("getLatticeIndexNT",
        [](const char *lattice) { return smolGetLatticeIndexNT(cursim_, lattice); });

    // char *smolGetLatticeName(simptr sim, int latticeindex, char *lattice);
    m.def("getLatticeName", [](int latticeindex, char *lattice) {
        return smolGetLatticeName(cursim_, latticeindex, lattice);
    });

    // enum ErrorCode smolAddLatticeMolecules(simptr sim, const char *lattice,
    //     const char *species, int number, double *lowposition,
    //     double *highposition);
    m.def("addLatticeMolecules",
        [](const char *lattice, const char *species, int number,
            vector<double> &lowposition, vector<double> &highposition) {
            return smolAddLatticeMolecules(
                cursim_, lattice, species, number, &lowposition[0], &highposition[0]);
        });

    // enum ErrorCode smolAddLatticeReaction(
    //     simptr sim, const char *lattice, const char *reaction, const int
    //     move);
    m.def("addLatticeReaction",
        [](const char *lattice, const char *reaction, const int move) {
            return smolAddLatticeReaction(cursim_, lattice, reaction, move);
        });

    /*********************************************************
     *  Extra function which are not avilable in the C-API.  *
     *********************************************************/
    m.def("getDim", &getDim, "Dimention of the system");
    m.def("setDim", &setDim, "Set the dimnetion of the system.");
    m.def("getSeed", &getRandomSeed);
    m.def("setSeed", &setRandomSeed);
    m.def("getBoundaries", &getBoundaries);
    m.def("setBoundaries", &setBoundaries);
    m.def("getDt", &getDt);
    m.def("setDt", &setDt);
    m.def("setAccuracy", [](double accuracy) { cursim_->accur = accuracy; });
    m.def("getAccuracy", [](void) { return cursim_->accur; });
    m.def("getData", &getData, py::return_value_policy::reference);

    /* Function */
    m.def("load_model", &init_and_run, "filepath"_a, "args"_a = "",
        "Load model from a txt file");

    /* attributes */
    m.attr("__version__") = VERSION;  // Version is set by CMAKE
}
