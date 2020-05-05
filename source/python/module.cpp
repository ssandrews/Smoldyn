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
int init_and_run(const string &filepath, const string &flags)
{
    int er = 0, wflag = 0;

    string filename, fileroot;
    auto   pos = filepath.find_last_of('/');

    fileroot = filepath.substr(0, pos + 1);
    filename = filepath.substr(pos + 1);

#ifdef OPTION_VCELL
    er = simInitAndLoad(fileroot.c_str(), filename.c_str(), &pSim_,
        flags.c_str(), new SimpleValueProviderFactory(), new SimpleMesh());
#else
    er = simInitAndLoad(
        fileroot.c_str(), filename.c_str(), &pSim_, flags.c_str());
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
    // py::options options;
    // options.disable_function_signatures();

    m.doc() = R"pbdoc(
        Python interface of smoldyn simulation.
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

    /**************************************************************************
     * FUNCTIONS.
     **************************************************************************
     */

    m.def("setDebugMode", &smolSetDebugMode);
    m.def("errorCodeToString", &smolErrorCodeToString);

    // Sim structure functions are not exposed to Python. They are for internal
    // use only.

    // Read configuration file.
    m.def("loadSimFromFile",
        [](const string &filepath, const char *flags) -> ErrorCode {
            string filename, fileroot;
            auto   pos = filepath.find_last_of('/');
            fileroot   = filepath.substr(0, pos + 1);
            filename   = filepath.substr(pos + 1);
            assert(pSim_);
            return smolLoadSimFromFile(
                fileroot.c_str(), filename.c_str(), &pSim_, flags);
        });
    m.def("readConfigString",
        [](const char *statement, char *params) -> ErrorCode {
            return smolReadConfigString(pSim_, statement, params);
        });

    // convenient wrappers.
    m.def("getDim", &getDim, "Dimention of the system");
    m.def("setDim", &setDim, "Set the dimnetion of the system.");
    m.def("getSeed", &getRandomSeed);
    m.def("setSeed", &setRandomSeed);
    m.def("getBoundaries", &getBoundaries);
    m.def("setBoundaries", &setBoundaries);

    /* box/molperbox etc */
    m.def("setPartitions", &setPartitions);

    /***************************** Molecules *******************************/
    m.def("addSpecies", &addSpecies, "name"_a, "mollist"_a = "");

    // int   smolGetSpeciesIndex(simptr sim, const char *species);
    m.def("getSpeciesIndex", [](const char *species) -> int {
        return smolGetSpeciesIndex(pSim_, species);
    });

    // int   smolGetSpeciesIndexNT(simptr sim, const char *species);
    m.def("speciesIndexNT", [](const char *species) {
        return smolGetSpeciesIndexNT(pSim_, species);
    });

    // char *smolGetSpeciesName(simptr sim, int speciesindex, char *species);
    m.def("getSpeciesName", [](int speciesindex, char *species) {
        return smolGetSpeciesName(pSim_, speciesindex, species);
    });

    // enum ErrorCode smolSetSpeciesMobility(simptr sim, const char *species,
    //     enum MolecState state, double difc, double *drift, double
    //     *difmatrix);
    m.def("setSpeciesMobility", &setSpeciesMobility, "species"_a, "state"_a,
        "diffConst"_a, "drift"_a = std::vector<double>(),
        "difmatrix"_a = std::vector<double>());

    //?? needs function smolSetSpeciesSurfaceDrift
    // enum ErrorCode smolAddMolList(simptr sim, const char *mollist);
    m.def("getMolListIndex", [](const char *mollist) {
        return smolGetMolListIndex(pSim_, mollist);
    });

    // int   smolGetMolListIndex(simptr sim, const char *mollist);
    m.def("getMolListIndex", [](const char *mollist) {
        return smolGetMolListIndex(pSim_, mollist);
    });

    // int   smolGetMolListIndexNT(simptr sim, const char *mollist);
    m.def("molListIndexNT", [](const char *mollist) {
        return smolGetMolListIndexNT(pSim_, mollist);
    });

    // char *smolGetMolListName(simptr sim, int mollistindex, char *mollist);
    m.def("getMolListName", [](int mollistindex, char *species) {
        return smolGetMolListName(pSim_, mollistindex, species);
    });

    // enum ErrorCode smolSetMolList(simptr sim, const char *species, enum
    // MolecState state, const char *mollist);
    m.def("setMolList",
        [](const char *species, MolecState state, const char *mollist) {
            return smolSetMolList(pSim_, species, state, mollist);
        });

    // enum ErrorCode smolSetMaxMolecules(simptr sim, int maxmolecules);
    m.def("setMaxMolecules", [](int maxmolecules) {
        return smolSetMaxMolecules(pSim_, maxmolecules);
    });

    // enum ErrorCode smolAddSolutionMolecules(simptr sim, const char *species,
    //     int number, double *lowposition, double *highposition);
    m.def("addSolutionMolecules",
        [](const char *species, int number, vector<double> &lowposition,
            vector<double> &highposition) {
            return smolAddSolutionMolecules(
                pSim_, species, number, &lowposition[0], &highposition[0]);
        });

    // enum ErrorCode smolAddCompartmentMolecules(
    //     simptr sim, const char *species, int number, const char
    //     *compartment);
    m.def("addCompartmentMolecules", [](const char *species, int number,
                                         const char *compartment) {
        return smolAddCompartmentMolecules(pSim_, species, number, compartment);
    });

    // enum ErrorCode smolAddSurfaceMolecules(simptr sim, const char *species,
    //     enum MolecState state, int number, const char *surface,
    //     enum PanelShape panelshape, const char *panel, double *position);
    m.def("addSurfaceMolecules",
        [](const char *species, MolecState state, int number,
            const char *surface, PanelShape panelshape, const char *panel,
            vector<double> &position) -> ErrorCode {
            return smolAddSurfaceMolecules(pSim_, species, state, number,
                surface, panelshape, panel, &position[0]);
        });

    // int smolGetMoleculeCount(simptr sim, const char *species, enum MolecState
    // state);
    m.def("getMoleculeCount", [](const char *species, MolecState state) {
        return smolGetMoleculeCount(pSim_, species, state);
    });

    // enum ErrorCode smolSetMoleculeStyle(simptr sim, const char *species,
    //     enum MolecState state, double size, double *color);
    m.def("setSpeciesStyle", &setSpeciesStyle);
    m.def("setMoleculeStyle",
        [](const char *species, MolecState state, double size, char *color) {
            array<double, 4> rgba = {0, 0, 0, 1.0};
            graphicsreadcolor(&color, &rgba[0]);
            return smolSetMoleculeStyle(pSim_, species, state, size, &rgba[0]);
        });

    /******************************* Surfaces ********************************/
    m.def("addSurface", &addSurface, "name"_a);
    m.def("setSurfaceAction", &setSurfaceAction);
    m.def("addSurfaceMolecules", &addSurfaceMolecules);

    // enum ErrorCode smolSetBoundaryType(simptr sim, int dimension, int
    // highside, char type);
    m.def("setBoundaryType", [](int dimension, int highside, char type) {
        return smolSetBoundaryType(pSim_, dimension, highside, type);
    });

    // enum ErrorCode smolAddSurface(simptr sim, const char *surface);
    m.def("addSurface",
        [](const char *surface) { return smolAddSurface(pSim_, surface); });

    // int molGetSurfaceIndex(simptr sim, const char *surface);
    m.def("getSurfaceIndex", [](const char *surface) {
        return smolGetSurfaceIndex(pSim_, surface);
    });

    // int            smolGetSurfaceIndexNT(simptr sim, const char *surface);
    m.def("getSurfaceIndexNT", [](const char *surface) {
        return smolGetSurfaceIndexNT(pSim_, surface);
    });

    // char *smolGetSurfaceName(simptr sim, int surfaceindex, char *surface);
    m.def("getSurfaceName", [](int surfaceindex, char *surface) {
        return smolGetSurfaceName(pSim_, surfaceindex, surface);
    });

    // enum ErrorCode smolSetSurfaceAction(simptr sim, const char *surface,
    //     enum PanelFace face, const char *species, enum MolecState state,
    //     enum SrfAction action);
    m.def("setSurfaceAction",
        [](const char *surface, PanelFace face, const char *species,
            MolecState state, SrfAction action) {
            return smolSetSurfaceAction(
                pSim_, surface, face, species, state, action);
        });

    // enum ErrorCode smolSetSurfaceRate(simptr sim, const char *surface,
    //     const char *species, enum MolecState state, enum MolecState state1,
    //     enum MolecState state2, double rate, const char *newspecies,
    //     int isinternal);
    m.def("setSurfaceRate",
        [](const char *surface, const char *species, MolecState state,
            MolecState state1, MolecState state2, double rate,
            const char *newspecies, bool isinternal) {
            return smolSetSurfaceRate(pSim_, surface, species, state, state1,
                state2, rate, newspecies, isinternal);
        });
    // enum ErrorCode smolAddPanel(simptr sim, const char *surface,
    //     enum PanelShape panelshape, const char *panel, const char
    //     *axisstring, double *params);
    m.def("addPanel",
        [](const char *surface, PanelShape panelshape, const char *panel,
            const char *axisstring, vector<double> &params) {
            return smolAddPanel(
                pSim_, surface, panelshape, panel, axisstring, &params[0]);
        });
    // int            smolGetPanelIndex(simptr sim, const char *surface,
    //                enum PanelShape *panelshapeptr, const char *panel);
    m.def("getPanelIndex",
        [](const char *surface, PanelShape *panelshape, const char *panel) {
            return smolGetPanelIndex(pSim_, surface, panelshape, panel);
        });

    // int            smolGetPanelIndexNT(simptr sim, const char *surface,
    //                enum PanelShape *panelshapeptr, const char *panel);
    m.def("getPanelIndexNT",
        [](const char *surface, PanelShape *panelshape, const char *panel) {
            return smolGetPanelIndexNT(pSim_, surface, panelshape, panel);
        });

    // char*  smolGetPanelName(simptr sim, const char *surface, enum PanelShape
    // panelshape, int panelindex, char *panel);
    m.def("getPanelName", [](const char *surface, PanelShape panelshape,
                              int panelindex, char *panel) {
        return smolGetPanelName(pSim_, surface, panelshape, panelindex, panel);
    });

    // enum ErrorCode smolSetPanelJump(simptr sim, const char *surface,
    //     const char *panel1, enum PanelFace face1, const char *panel2,
    //     enum PanelFace face2, int isbidirectional);
    m.def("setPanelJump",
        [](const char *surface, const char *panel1, PanelFace face1,
            const char *panel2, PanelFace face2, bool isbidirectional) {
            return smolSetPanelJump(
                pSim_, surface, panel1, face1, panel2, face2, isbidirectional);
        });

    // enum ErrorCode smolAddSurfaceUnboundedEmitter(simptr sim,
    //     const char *surface, enum PanelFace face, const char *species,
    //     double emitamount, double *emitposition);
    m.def("addSurfaceUnboundedEmitter",
        [](const char *surface, PanelFace face, const char *species,
            double emitamount, vector<double> &emitposition) {
            return smolAddSurfaceUnboundedEmitter(
                pSim_, surface, face, species, emitamount, &emitposition[0]);
        });

    // enum ErrorCode smolSetSurfaceSimParams(
    //     simptr sim, const char *parameter, double value);
    m.def("setSurfaceSimParams", [](const char *parameter, double value) {
        return smolSetSurfaceSimParams(pSim_, parameter, value);
    });

    // enum ErrorCode smolAddPanelNeighbor(simptr sim, const char *surface1,
    //     const char *panel1, const char *surface2, const char *panel2,
    //     int reciprocal);
    m.def("addPanelNeighbhor",
        [](const char *surface1, const char *panel1, const char *surface2,
            const char *panel2, int reciprocal) {
            return smolAddPanelNeighbor(
                pSim_, surface1, panel1, surface2, panel2, reciprocal);
        });

    // enum ErrorCode smolSetSurfaceStyle(simptr sim, const char *surface,
    //     enum PanelFace face, enum DrawMode mode, double thickness,
    //     double *color, int stipplefactor, int stipplepattern, double
    //     shininess);
    m.def("setSurfaceStyle",
        [](const char *surface, PanelFace face, DrawMode mode, double thickness,
            char *color, int stipplefactor, int stipplepattern,
            double shininess) {
            array<double, 4> rgba = {0, 0, 0, 1.0};
            graphicsreadcolor(&color, &rgba[0]);
            return smolSetSurfaceStyle(pSim_, surface, face, mode, thickness,
                &rgba[0], stipplefactor, stipplepattern, shininess);
        });

    /*** Comparment functions. ***/
    // enum ErrorCode smolAddCompartment(simptr sim, const char *compartment);
    m.def("addCompartment", [](const char *compartment) {
        return smolAddCompartment(pSim_, compartment);
    });

    // int smolGetCompartmentIndex(simptr sim, const char *compartment);
    m.def("getCompartmentIndex", [](const char *compartment) {
        return smolGetCompartmentIndex(pSim_, compartment);
    });

    // int smolGetCompartmentIndexNT(simptr sim, const char *compartment);
    m.def("getCompartmentIndexNT", [](const char *compartment) {
        return smolGetCompartmentIndexNT(pSim_, compartment);
    });
    // char * smolGetCompartmentName(simptr sim, int compartmentindex, char
    // *compartment);
    m.def("getCompartmentName", [](int compartmentindex, char *compartment) {
        return smolGetCompartmentName(pSim_, compartmentindex, compartment);
    });

    // enum ErrorCode smolAddCompartmentSurface(simptr sim, const char
    // *compartment, const char *surface);

    // enum ErrorCode smolAddCompartmentPoint(
    //         simptr sim, const char *compartment, double *point);
    // enum ErrorCode smolAddCompartmentLogic(simptr sim, const char
    // *compartment,
    //         enum CmptLogic logic, const char *compartment2);

    /* Panel */
    m.def("addPanel", &addPanel);

    /* Compartment */
    m.def("addCompartment", &addCompartment);
    m.def("addCompartmentSurface", &addCompartmentSurface);
    m.def("addCompartmentPoint", &addCompartmentPoint);
    m.def("addComparmentMolecules", &addCompartmentMolecules);
    m.def("addSolutionMolecules", &addSolutionMolecules);

    /* Reaction */
    m.def("addReaction", &addReaction, "reac"_a, "reactant1"_a, "rstate1"_a,
        "reactant2"_a, "rstate2"_a, "products"_a, "productstates"_a, "rate"_a);
    m.def("setReactionRegion", &setReactionRegion);
    m.def("setBoundaryType", &setBoundaryType);
    m.def("addMolList", &addMolList);

    /* data */
    m.def("getMoleculeCount", &getMoleculeCount);

    /* Graphics */
    m.def("setGraphicsParams", &setGraphicsParams);
    m.def("setGraphicsParams",
        [](const char *method, int timestep, int delay) -> ErrorCode {
            return smolSetGraphicsParams(pSim_, method, timestep, delay);
        });

    m.def("setTiffParams",
        [](int timesteps, const char *tiffname, int lowcount,
            int highcount) -> ErrorCode {
            return smolSetTiffParams(
                pSim_, timesteps, tiffname, lowcount, highcount);
        });

    m.def("setLightParams",
        [](int lightindex, vector<double> &ambient, vector<double> &diffuse,
            vector<double> &specular, vector<double> &position) -> ErrorCode {
            return smolSetLightParams(pSim_, lightindex, &ambient[0],
                &diffuse[0], &specular[0], &position[0]);
        });

    m.def("setBackgroundStyle", [](char *color) -> ErrorCode {
        array<double, 4> rgba = {0, 0, 0, 1.0};
        graphicsreadcolor(&color, &rgba[0]);
        return smolSetBackgroundStyle(pSim_, &rgba[0]);
    });

    m.def("setFrameStyle", [](double thickness, char *color) -> ErrorCode {
        array<double, 4> rgba = {0, 0, 0, 1.0};
        graphicsreadcolor(&color, &rgba[0]);
        return smolSetBackgroundStyle(pSim_, &rgba[0]);
    });

    m.def("setGridStyle", [](double thickness, char *color) {
        array<double, 4> rgba = {0, 0, 0, 1.0};
        graphicsreadcolor(&color, &rgba[0]);
        return smolSetGridStyle(pSim_, thickness, &rgba[0]);
    });

    m.def("setTextStyle", [](char *color) -> ErrorCode {
        array<double, 4> rgba = {0, 0, 0, 1.0};
        graphicsreadcolor(&color, &rgba[0]);
        return smolSetTextStyle(pSim_, &rgba[0]);
    });

    m.def("addTextDisplay", [](char *item) -> ErrorCode {
        return smolAddTextDisplay(pSim_, item);
    });

    /* Simulation */
    m.def("setSimTimes",
        [](double timestart, double timestop, double timestep) -> ErrorCode {
            return smolSetSimTimes(pSim_, timestart, timestop, timestep);
        });

    m.def("setTimeStart",
        [](double time) -> ErrorCode { return smolSetTimeStart(pSim_, time); });

    m.def("setTimeStop", [](double timestop) -> ErrorCode {
        return smolSetTimeStop(pSim_, timestop);
    });

    m.def("setTimeNow", [](double timenow) -> ErrorCode {
        return smolSetTimeNow(pSim_, timenow);
    });

    m.def("setTimeStep", [](double timestep) -> ErrorCode {
        return smolSetTimeStep(pSim_, timestep);
    });

    m.def("setRandomSeed", [](long int seed) -> ErrorCode {
        return smolSetRandomSeed(pSim_, seed);
    });

    m.def("setSimTimes", &setSimTimes);
    m.def("updateSim", &updateSim);
    m.def("runSim", &runSim, "stoptime"_a, "dt"_a, "display"_a = true);
    m.def(
        "runUntil", &runUntil, "breaktime"_a, "dt"_a = 0.0, "display"_a = true);
    m.def("getDt", &getDt);
    m.def("setDt", &setDt);

    // See
    // https://pybind11.readthedocs.io/en/stable/advanced/misc.html#module-destructors
    // for details.
    m.add_object("_cleanup", py::capsule(cleanup));

    /* Function */
    m.def("load_model", &init_and_run, "filepath"_a, "args"_a = "",
        "Load model from a txt file");

    /* attributes */
    m.attr("__version__") = VERSION;  // Version is set by CMAKE
}
