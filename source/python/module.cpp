/*
 * Python module.
 * Author: Dilawar Singh <dilawar.s.rajput@gmail.com>
 */

#include <iostream>
#include <vector>
#include <string>
#include <exception>
#include <map>

using namespace std;

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"
#include "pybind11/functional.h"

// Globals are also declarated here.

#include "../libSteve/SimCommand.h"
#include "../Smoldyn/smoldynfuncs.h"
#include "Smoldyn.h"
#include "CallbackFunc.h"

using namespace pybind11::literals;  // for _a

double r_ = 0.0;

/* globals */
vector<unique_ptr<CallbackFunc>> callbacks_;

/* gl2glutInit */
extern void gl2glutInit(int *argc, char **argv);

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  Convert an array of color to a tuple.
 *
 * @Param f
 *
 * @Returns
 */
/* ----------------------------------------------------------------------------*/
array<double, 4> pycolor(const double *f)
{
    array<double, 4> c = {f[0], f[1], f[2], f[3]};
    return c;
}

/* --------------------------------------------------------------------------*/
/** @Synopsis  Initialize and run a given model file.
 *
 * This function takes input file and run the simulation. This is more or less
 * same as `main()` function in smoldyn.c file.
 *
 * @Param filepath
 * @Param flags
 * @Param wflag If 1, overwrite exsiting files.
 *
 */
/* ----------------------------------------------------------------------------*/
int init_and_run(
    const string &filepath, const string &flags, bool wflag, bool quit_at_end = false)
{
    int  er = 0;
    auto p  = splitPath(filepath);

#ifdef OPTION_VCELL
    er = simInitAndLoad(p.first.c_str(), p.second.c_str(), &cursim_, flags.c_str(),
        new SimpleValueProviderFactory(), new SimpleMesh());
#else
    er = simInitAndLoad(p.first.c_str(), p.second.c_str(), &cursim_, flags.c_str());
#endif
    if(!er) {
        cursim_->quitatend = quit_at_end;
        if(cursim_->graphss && cursim_->graphss->graphics != 0) gl2glutInit(0, nullptr);
        er = simUpdateAndDisplay(cursim_);
    }
    if(!er) er = smolOpenOutputFiles(cursim_, wflag);
    if(er) {
        simLog(cursim_, 4, "%sSimulation skipped\n", er ? "\n" : "");
    }
    else {
        fflush(stdout);
        fflush(stderr);
        if(!cursim_->graphss || cursim_->graphss->graphics == 0) {
            er = smolsimulate(cursim_);
            endsimulate(cursim_, er);
        }
        else {
            smolsimulategl(cursim_);
        }
    }
    simfree(cursim_);
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
        .value("fsoln", MolecState::MSsoln)  // fsoln is really soln
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

    py::enum_<DrawMode>(m, "DrawMode")
        .value("no", DrawMode::DMno)
        .value("vert", DrawMode::DMvert)
        .value("edge", DrawMode::DMedge)
        .value("ve", DrawMode::DMve)
        .value("face", DrawMode::DMface)
        .value("vf", DrawMode::DMvf)
        .value("ef", DrawMode::DMef)
        .value("vef", DrawMode::DMvef)
        .value("none", DrawMode::DMnone);

    py::enum_<SMLflag>(m, "SMLflag")
        .value("no", SMLflag::SMLno)
        .value("diffuse", SMLflag::SMLdiffuse)
        .value("react", SMLflag::SMLreact)
        .value("srfbound", SMLflag::SMLsrfbound);

    py::enum_<RevParam>(m, "RevParam")
        .value("none", RevParam::RPnone)
        .value("irrev", RevParam::RPirrev)
        .value("confspread", RevParam::RPconfspread)
        .value("bounce", RevParam::RPbounce)
        .value("pgem", RevParam::RPpgem)
        .value("pgemmax", RevParam::RPpgemmax)
        .value("pgemmaxw", RevParam::RPpgemmaxw)
        .value("ratio", RevParam::RPratio)
        .value("unbindrad", RevParam::RPunbindrad)
        .value("pgem2", RevParam::RPpgem2)
        .value("pgemmax2", RevParam::RPpgemmax2)
        .value("ratio2", RevParam::RPratio2)
        .value("offset", RevParam::RPoffset)
        .value("fixed", RevParam::RPfixed);

    py::enum_<SpeciesRepresentation>(m, "SpeciesRepresentation")
        .value("particle", SpeciesRepresentation::SRparticle)
        .value("lattice", SpeciesRepresentation::SRlattice)
        .value("both", SpeciesRepresentation::SRboth)
        .value("none", SpeciesRepresentation::SRnone)
        .value("free", SpeciesRepresentation::SRfree);

    /* callback */
    py::class_<CallbackFunc>(m, "CallbackFunc")
        .def(py::init<>())
        .def_property("func", &CallbackFunc::getFuncName, &CallbackFunc::setFuncName)
        .def("evalAndUpdate", &CallbackFunc::evalAndUpdate);

    /* graphics */
    py::class_<graphicssuperstruct>(m, "Graphics")
        .def_readonly(
            "condition", &graphicssuperstruct::condition)  // structure condition
        .def_readonly("method",
            &graphicssuperstruct::graphics)  // graphics: 0=none, 1=opengl, 2=good opengl
        .def_readonly(
            "runmode", &graphicssuperstruct::runmode)  // 0=Smoldyn, 1=Libsmoldyn
        .def_readonly("currentIter",
            &graphicssuperstruct::currentit)  // current number of simulation time steps
        .def_readonly("graphicIter",
            &graphicssuperstruct::graphicit)  // number of time steps per graphics update
        .def_readonly(
            "graphicDelay", &graphicssuperstruct::graphicdelay)  // minimum delay (in ms)
                                                                 // for graphics updates
        .def_readonly("tiffIter",
            &graphicssuperstruct::tiffit)  // number of time steps per tiff save
        .def_readonly("framePoints",
            &graphicssuperstruct::framepts)  // thickness of frame for graphics
        .def_readonly("gridPoints",
            &graphicssuperstruct::gridpts)  // thickness of virtual box grid for graphics
        .def_readonly("frameColor", &graphicssuperstruct::framecolor)  // frame color [c]
        .def_property_readonly("gridColor",
            [](const graphicssuperstruct &st) {
                return pycolor(st.gridcolor);
            })  // grid color [c]
        .def_property_readonly("bgColor",
            [](const graphicssuperstruct &st) {
                return pycolor(st.backcolor);
            })  // background color [c]
        .def_property_readonly("textColor",
            [](const graphicssuperstruct &st) {
                return pycolor(st.textcolor);
            })  // text color [c]
        .def_readonly("maxTextItems",
            &graphicssuperstruct::maxtextitems)  // allocated size of item list
        .def_readonly(
            "nTextItems", &graphicssuperstruct::ntextitems)  // actual size of item list
        .def_property_readonly("textItems",
            [](const graphicssuperstruct &gst) {
                vector<std::string> txt(gst.ntextitems, "");
                for(int i = 0; i < gst.ntextitems; i++) {
                    const char *t = gst.textitems[i];
                    txt[i]        = std::string(t);
                }
                return txt;
            })
        // .def_readonly(
        //     "roomstate", &graphicssuperstruct::roomstate)  // on, off, or auto (on)
        .def_property_readonly("globalAmbientLightColor",
            [](const graphicssuperstruct &st) {
                return pycolor(st.ambiroom);
            })  // global ambient light [c]
        // .def_readonly("lightstate",
        //     &graphicssuperstruct::lightstate)  // on, off, or auto (off) [lt]
        .def_property_readonly("ambientLightColor",
            [](const graphicssuperstruct &st) {
                vector<array<double, 4>> ambient(MAXLIGHTS);
                for(size_t i = 0; i < MAXLIGHTS; i++)
                    ambient[i] = pycolor(st.ambilight[i]);
                return ambient;
            })
        .def_property_readonly("diffuseLightColor",
            [](const graphicssuperstruct &st) {
                vector<array<double, 4>> difflight(MAXLIGHTS);
                for(size_t i = 0; i < MAXLIGHTS; i++)
                    difflight[i] = pycolor(st.difflight[i]);
                return difflight;
            })  // diffuse light color [lt][c]
        .def("specularLightColor",
            [](const graphicssuperstruct &st) {
                vector<array<double, 4>> light(MAXLIGHTS);
                for(size_t i = 0; i < MAXLIGHTS; i++) light[i] = pycolor(st.speclight[i]);
                return light;
            })  // specular light color [lt][c]
        .def("lightPosition",
            [](const graphicssuperstruct &st, size_t lindex) {
                return st.lightpos[lindex];
            })  // light positions [lt][d]
        ;

    /* simptr */
    py::class_<simstruct>(m, "simstruct")
        //.def(py::init<>())
        .def_readonly("condition", &simstruct::condition, "Structure condition")

        // Python user can write these values.
        .def_readwrite("logfile", &simstruct::logfile, "configuration file path")
        .def_readwrite("filepath", &simstruct::filepath, "configuration file path")
        .def_readwrite("filename", &simstruct::filename, "configuration file name")
        .def_readwrite("flags", &simstruct::flags, "command-line options from user")
        .def_readwrite(
            "quit_at_end", &simstruct::quitatend, "simulation quits at the end")

        // These are readonly.
        .def_readonly(
            "start_time", &simstruct::clockstt, "clock starting time of simulation")
        .def_readonly(
            "elapsed_time", &simstruct::elapsedtime, "elapsed time of simulation")
        .def_readonly("randseed", &simstruct::randseed, "random number generator seed")
        .def_readonly(
            "event_count", &simstruct::eventcount, "counter for simulation events")
        .def_readonly("max_var", &simstruct::maxvar, "allocated user-settable variables")
        .def_readonly("nvar", &simstruct::nvar, "number of user-settable variables")
        .def_readonly("dim", &simstruct::dim, "dimensionality of space.")
        .def_readonly("accuracy", &simstruct::accur, "accuracy, on scale from 0 to 10")
        .def_readonly("time", &simstruct::time, "current time in simulation")
        .def_readonly("start_time", &simstruct::tmin, "simulation start time")
        .def_readonly("end_time", &simstruct::tmax, "simulation end time")
        .def_readonly("break_time", &simstruct::tbreak, "simulation break time")
        .def_readonly("dt", &simstruct::dt, "simulation time step")
        .def_readonly("rules", &simstruct::ruless, "rule superstructure")
        .def_readonly("molecuels", &simstruct::mols, "molecule superstructure")
        .def_readonly("surfaces", &simstruct::srfss, "surface superstructure")
        .def_readonly("boxs", &simstruct::boxs, "box superstructure")
        .def_readonly("cmpts", &simstruct::cmptss, "compartment superstructure")
        .def_readonly("ports", &simstruct::portss, "port superstructure")
        .def_readonly("lattices", &simstruct::latticess, "lattice superstructure")
        .def_readonly("bionets", &simstruct::bngss, "bionetget superstructure")
        .def_readonly("filaments", &simstruct::filss, "filament superstructure")
        .def_readonly("commands", &simstruct::cmds, "command superstructure")
        .def_readonly(
            "graphics", &simstruct::graphss, py::return_value_policy::reference);

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
        [](vector<double> &lowbounds, vector<double> &highbounds) -> simptr {
            auto sim = smolNewSim(lowbounds.size(), &lowbounds[0], &highbounds[0]);
            simptrs_.push_back(sim);
            return simptrs_.back();
        },
        py::return_value_policy::reference);

    // make a simptr current working simptr. The simptr must be initialized
    // properly.
    // FIXME: Tests are not exhaustive.
    m.def(
        "setCurSimStruct", [](simstruct *const ptr) { cursim_ = ptr; },
        "change current simstruct");
    m.def(
        "getCurSimStruct",
        []() {
            if(!cursim_) {
                py::print(
                    "Warn: Simultion config (simptr) is not initialize yet."
                    " Did you set the `dim` and `boundary`?");
            }
            return cursim_;
        },
        "Get current sim struct", py::return_value_policy::reference);

    m.def("updateSim", [](void) {
        if(!cursim_) {
            py::print(
                "Warn: Simulation is not initialized yet. Did you set `boundaries` and "
                "`dim`?");
            return ECwarning;
        }
        return smolUpdateSim(cursim_);
    });
    m.def(
        "clearAllSimStructs",
        []() {
            for(auto v : simptrs_) deleteSimptr(v);
            simptrs_.clear();
            deleteSimptr(cursim_);
        },
        "Clear all simptrs (excluding the one in use)");
    m.def("runTimeStep", [](void) { return smolRunTimeStep(cursim_); });
    m.def("runSim", [](void) { return smolRunSim(cursim_); });
    m.def("runSimUntil", [](double breaktime, bool overwrite) {
        return smolRunSimUntil(cursim_, breaktime);
    });

    m.def("freeSim", [](void) { return smolFreeSim(cursim_); });
    m.def("displaySim", [](void) { return smolDisplaySim(cursim_); });

    // Extra (not in C api).
    m.def("run", &runSimulation, "stoptime"_a, "dt"_a, "display"_a = true,
        "overwrite"_a = false);
    m.def("runUntil", &runUntil, "breaktime"_a, "dt"_a = 0.0, "display"_a = true,
        "overwrite"_a = false);

    /*****************************
     *  Read configuration file  *
     *****************************/
    m.def("prepareSimFromFile", [](const char *filepath, const char *flags) {
        auto path = splitPath(string(filepath));
        return smolPrepareSimFromFile(path.first.c_str(), path.second.c_str(), flags);
    });
    m.def("loadSimFromFile", [](const string &filepath, const char *flags) {
        auto p = splitPath(filepath);
        return smolLoadSimFromFile(p.first.c_str(), p.second.c_str(), &cursim_, flags);
    });
    m.def("readConfigString", [](const char *statement, char *params) {
        return smolReadConfigString(cursim_, statement, params);
    });

    /*************************
     *  Simulation settings  *
     *************************/
    // enum ErrorCode smolSetSimTimes(
    //         simptr sim, double timestart, double timestop, double timestep);
    m.def("setSimTimes", [](double timestart, double timestop, double timestep) {
        return smolSetSimTimes(cursim_, timestart, timestop, timestep);
    });

    // enum ErrorCode smolSetTimeStart(simptr sim, double timestart);
    m.def("setTimeStart", [](double time) { return smolSetTimeStart(cursim_, time); });
    m.def("getTimeStart", []() { return cursim_->tmin; });

    // enum ErrorCode smolSetTimeStop(simptr sim, double timestop);
    m.def("setTimeStop",
        [](double timestop) { return smolSetTimeStop(cursim_, timestop); });
    m.def("getTimeStop", []() { return cursim_->tmax; });

    // enum ErrorCode smolSetTimeNow(simptr sim, double timenow);
    m.def("setTimeNow", [](double timenow) { return smolSetTimeNow(cursim_, timenow); });

    // enum ErrorCode smolSetTimeStep(simptr sim, double timestep);
    m.def("setTimeStep",
        [](double timestep) { return smolSetTimeStep(cursim_, timestep); });
    m.def("getTimeStep", []() { return cursim_->dt; });

    m.def(
        "setRandomSeed", [](long int seed) { return smolSetRandomSeed(cursim_, seed); });

    // enum ErrorCode smolSetPartitions(simptr sim, const char *method, double
    // value);
    m.def(
        "setPartitions",
        [](const char *method, double value) {
            return smolSetPartitions(cursim_, method, value);
        },
        R"""( Sets the
            virtual partitions in the simulation volume. Enter method as
            molperbox and then enter value with the requested number of
            molecules per partition volume; the default, which is used if this
            function is not called at all, is a target of 4 molecules per box.
            Or, enter method as boxsize and enter value with the requested
            partition spacing. In this latter case, the actual partition
            spacing may be larger or smaller than the requested value in order
            to fit an integer number of partitions into each coordinate of the
            simulation volume.)""");

    /*********************************
     *  Graphics related functions.  *
     *********************************/
    // enum ErrorCode smolSetGraphicsParams(simptr sim, const char *method, int
    // timesteps, int delay);
    m.def(
        "setGraphicsParams",
        [](const char *method, int timestep, int delay) {
            return smolSetGraphicsParams(cursim_, method, timestep, delay);
        },
        "Sets basic simulation graphics parameters. Enter method as \"\" for no graphics "
        "(the default), \"opengl\" for fast but minimal OpenGL graphics, \"opengl_good\" "
        "for "
        "improved OpenGL graphics, \"opengl_better\" for fairly good OpenGL graphics, or "
        "as "
        "\"nullptr\" to not set this parameter currently. Enter timesteps with a "
        "positive "
        "integer to set the number of simulation time steps between graphics renderings "
        "(1 is the default) or with a negative number to not set this parameter "
        "currently. Enter delay as a non-negative number to set the minimum number of "
        "milliseconds that must elapse between subsequent graphics renderings in order "
        "to improve visualization (0 is the default) or as a negative number to not set "
        "this parameter currently.");

    // enum ErrorCode smolSetTiffParams(simptr sim, int timesteps,
    //     const char *tiffname, int lowcount, int highcount);
    m.def(
        "setTiffParams",
        [](int timesteps, const char *tiffname, int lowcount, int highcount) {
            return smolSetTiffParams(cursim_, timesteps, tiffname, lowcount, highcount);
        },
        "Sets parameters for the automatic collection of TIFF format snapshots of the "
        "graphics window. timesteps is the number of simulation timesteps that should "
        "elapse between subsequent snapshots, tiffname is the root filename of the "
        "output TIFF files, lowcount is a number that is appended to the filename of the "
        "first snapshot and which is then incremented for subsequent snapshots, and "
        "highcount is the last numbered file that will be collected. Enter negative "
        "numbers for timesteps, lowcount, and/or highcount to not set these parameters, "
        "and enter NULL for tiffname to not set the file name."

    );

    // enum ErrorCode smolSetLightParams(simptr sim, int lightindex, double
    // *ambient,
    //     double *diffuse, double *specular, double *position);
    m.def("setLightParams",
        [](int lightindex, vector<double> &ambient, vector<double> &diffuse,
            vector<double> &specular, vector<double> &position) {
            return smolSetLightParams(cursim_, lightindex, &ambient[0], &diffuse[0],
                &specular[0], &position[0]);
        });

    // enum ErrorCode smolSetBackgroundStyle(simptr sim, double *color);
    m.def("setBackgroundStyle", [](char *color) {
        array<double, 4> rgba = {0, 0, 0, 1.0};
        graphicsreadcolor(&color, &rgba[0]);
        // cout << "debug: Setting background color " << rgba[0] << ' ' << rgba[1] << ' '
        // << rgba[2] << ' ' << rgba[3] << endl;
        return smolSetBackgroundStyle(cursim_, &rgba[0]);
    });
    m.def("setBackgroundStyle",
        [](array<double, 4> rgba) { return smolSetBackgroundStyle(cursim_, &rgba[0]); });

    // enum ErrorCode smolSetFrameStyle(simptr sim, double thickness, double
    // *color);
    m.def("setFrameStyle", [](double thickness, char *color) {
        array<double, 4> rgba = {0, 0, 0, 1.0};
        graphicsreadcolor(&color, &rgba[0]);
        return smolSetFrameStyle(cursim_, thickness, &rgba[0]);
    });

    m.def("setFrameStyle", [](double thickness, array<double, 4> &rgba) {
        return smolSetFrameStyle(cursim_, thickness, &rgba[0]);
    });

    // enum ErrorCode smolSetGridStyle(simptr sim, double thickness, double
    // *color);
    m.def("setGridStyle", [](double thickness, char *color) {
        array<double, 4> rgba = {0, 0, 0, 1.0};
        graphicsreadcolor(&color, &rgba[0]);
        return smolSetGridStyle(cursim_, thickness, &rgba[0]);
    });

    m.def("setGridStyle", [](double thickness, array<double, 4> &rgba) {
        return smolSetGridStyle(cursim_, thickness, &rgba[0]);
    });

    // enum ErrorCode smolSetTextStyle(simptr sim, double *color);
    m.def("setTextStyle", [](char *color) {
        array<double, 4> rgba = {0, 0, 0, 1.0};
        graphicsreadcolor(&color, &rgba[0]);
        return smolSetTextStyle(cursim_, &rgba[0]);
    });
    m.def("setTextStyle",
        [](array<double, 4> &rgba) { return smolSetTextStyle(cursim_, &rgba[0]); });

    // enum ErrorCode smolAddTextDisplay(simptr sim, char *item);
    m.def("addTextDisplay", [](char *item) { return smolAddTextDisplay(cursim_, item); });

    /***********************
     *  Runtime commands.  *
     ***********************/
    // enum ErrorCode smolSetOutputPath(simptr sim, const char *path);
    m.def("setOutputPath",
        [](const char *path) { return smolSetOutputPath(cursim_, path); });

    // enum ErrorCode smolAddOutputFile(simptr sim, char *filename, int suffix,
    // int append);
    m.def("addOutputFile", [](char *filename, int suffix, bool append) {
        return smolAddOutputFile(cursim_, filename, suffix, append);
    });

    // enum ErrorCode smolAddOutputData(simptr sim, char *dataname);
    m.def("addOutputData",
        [](char *dataname) { return smolAddOutputData(cursim_, dataname); });

    // enum ErrorCode smolAddCommand(simptr sim, char type, double on, double
    // off,
    //     double step, double multiplier, const char *commandstring);
    m.def("addCommand", [](char type, double on, double off, double step,
                            double multiplier, const char *commandstring) {
        return smolAddCommand(cursim_, type, on, off, step, multiplier, commandstring);
    });

    // enum ErrorCode smolAddCommandFromString(simptr sim, char *string);
    m.def("addCommandFromString", [](char *cmd) {
        // char *cmd = strdup(command.c_str());
        return smolAddCommandFromString(cursim_, cmd);
    });

    // enum ErrorCode smolGetOutputData(simptr sim,char *dataname,int *nrow,int *ncol,
    // double **array,int erase)
    m.def("getOutputData", [](char *dataname, bool erase) -> vector<vector<double>> {
        int     nrow, ncol;
        double *array;

        smolGetOutputData(cursim_, dataname, &nrow, &ncol, &array, erase);
        std::vector<vector<double>> cppdata(nrow);
        for(int i = 0; i < nrow; i++)
            cppdata[i] = vector<double>(array + i * ncol, array + (i + 1) * ncol);
        free(array);
        return cppdata;
    });

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
    m.def(
        "addSolutionMolecules",
        [](const char *species, size_t number, vector<double> &lowposition,
            vector<double> &highposition) {
            return smolAddSolutionMolecules(
                cursim_, species, number, &lowposition[0], &highposition[0]);
        },
        "species"_a, "number"_a, "lowpos"_a = vector<double>(),
        "highpos"_a = vector<double>(),
        "Adds given number of molecules of a species to the system. They are "
        "randomly distributed within the box that has its opposite corners defined by "
        "`lowposition` and `highposition`. Any or all of these coordinates can equal "
        "each other to place the molecules along a plane or at a point. Enter "
        "`lowposition` and/or `highposition` as `[]` or `None` to indicate that the "
        "respective corner is equal to that corner of the entire system volume.");

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
            PanelShape panelshape, const char *panel, vector<double> &position) {
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
    m.def("setMoleculeStyle",
        [](const char *species, MolecState state, double size, array<double, 4> &rgba) {
            return smolSetMoleculeStyle(cursim_, species, state, size, &rgba[0]);
        });

    // enum ErrorCode smolSetMoleculeColor(simptr sim, const char *species,
    //     enum MolecState state, double *color);
    m.def("setMoleculeColor", [](const char *species, MolecState state, char *color) {
        auto rgba = color2RGBA(color);
        return smolSetMoleculeColor(cursim_, species, state, &rgba[0]);
    });

    m.def("setMoleculeColor",
        [](const char *species, MolecState state, array<double, 4> &rgba) {
            return smolSetMoleculeColor(cursim_, species, state, &rgba[0]);
        });

    // enum ErrorCode smolSetMoleculeSize(simptr sim, const char *species,
    //     enum MolecState state, double size);
    m.def("setMoleculeSize", [](const char *species, MolecState state, double size) {
        return smolSetMoleculeSize(cursim_, species, state, size);
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
    //     enum SrfAction action, const char *newspecies);
    m.def(
        "setSurfaceAction",
        [](const char *surface, PanelFace face, const char *species, MolecState state,
            SrfAction action, const char *newspecies) {
            return smolSetSurfaceAction(
                cursim_, surface, face, species, state, action, newspecies);
        },
        "surface"_a, "face"_a, "species"_a, "state"_a, "action"_a, "newspecies"_a = "");

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
    m.def("getPanelIndex", [](const char *surface, const char *panel) {
        PanelShape panelshape{PanelShape::PSnone};
        return smolGetPanelIndex(cursim_, surface, &panelshape, panel);
    });

    // int            smolGetPanelIndexNT(simptr sim, const char *surface,
    //                enum PanelShape *panelshapeptr, const char *panel);
    m.def("getPanelIndexNT", [](const char *surface, const char *panel) {
        PanelShape panelshape{PanelShape::PSnone};
        return smolGetPanelIndexNT(cursim_, surface, &panelshape, panel);
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
    m.def("addPanelNeighbor",
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
    m.def(
        "setSurfaceStyle", [](const char *surface, PanelFace face, DrawMode mode,
                               double thickness, array<double, 4> &rgba,
                               int stipplefactor, int stipplepattern, double shininess) {
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
            if(nprd >= MAXPRODUCT) {
                py::print("Maximum product allowed ", MAXPRODUCT, ". Given ", nprd);
                return ErrorCode::ECbounds;
            }

            vector<const char *> productSpecies(nprd);
            for(size_t i = 0; i < nprd; i++)
                productSpecies[i] = productSpeciesStr[i].c_str();

            return smolAddReaction(cursim_, reaction, reactant1, rstate1, reactant2,
                rstate2, productSpecies.size(), &productSpecies[0], &productStates[0],
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

    // enum ErrorCode smolSetReactionIntersurface(simptr sim, const char *reaction,
    //     int* rules);
    m.def("setReactionIntersurface", [](const char *reaction, vector<int> &rules) {
        return smolSetReactionIntersurface(cursim_, reaction, &rules[0]);
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
        for(auto &vec : pos) ptrs.push_back(vec.data());
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
    m.def("color2RGBA", &color2RGBA, "Convert a string to rgba tuple");
    m.def("getDim", &getDim, "Get dimensions of the system");
    m.def("setDim", &setDim, "Set dimensions of the system.");
    m.def("getRandomSeed", &getRandomSeed, "Get the seed of the internal RNG");
    m.def("getBoundaries", &getBoundaries);

    // Set model path on simptr
    m.def("setModelpath", &setModelpath, "modelpath"_a,
        "Set model path for current Simulation (for internal use)");

    // Set boundaries
    m.def("setBoundaries",
        py::overload_cast<const vector<pair<double, double>> &>(&setBoundaries),
        "Set the simulation boundaries using a vector of  tuples (low,high) e.g., "
        "[(xlow, xhigh), (ylow, yhigh), (zlow, zhigh)].");
    m.def("setBoundaries",
        py::overload_cast<vector<double> &, vector<double> &>(&setBoundaries),
        "Set the simulation boundaries using vectors of low and high points e.g., "
        "[xlow, ylow, zlow], [xhigh, yhigh, zhigh].");

    m.def("getDt", &getDt);
    m.def("setDt", &setDt);

    /* set accuracy */
    m.def("setAccuracy", [](double accuracy) { cursim_->accur = accuracy; });
    m.def("getAccuracy", [](void) { return cursim_->accur; });

    /* Function */
    m.def("loadModel", &init_and_run, "filepath"_a, "flags"_a = "", "wflag"_a = false,
        "quit_at_end"_a = false, "Load model from a txt file");

    /* Extra Function */
    m.def("connect", &connect, "func"_a, "target"_a, "step"_a = 1,
        "args"_a = std::vector<double>());

    /* attributes */
    m.attr("__version__") = VERSION;  // Version is set by CMAKE
}
