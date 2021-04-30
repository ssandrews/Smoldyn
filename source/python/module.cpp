/*
 * Python interfaces for Smoldyn simulator.
 * Author: Dilawar Singh <dilawar.s.rajput@gmail.com>
 */

#include <algorithm>
#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

#include "../Smoldyn/smoldynfuncs.h"
#include "../libSteve/SimCommand.h"
#include "CallbackFunc.h"
#include "Simulation.h"
#include "module.h"
#include "pybind11/functional.h"
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"

using namespace pybind11::literals; // for _a

double r_ = 0.0;
char tempstring[256];

/* --------------------------------------------------------------------------*/
/**
 * @Synopsis  Convert an array of color to a tuple.
 *
 * @Param f
 *
 * @Returns
 */
/* ----------------------------------------------------------------------------*/
array<double, 4>
pycolor(const double* f)
{
    array<double, 4> c = { f[0], f[1], f[2], f[3] };
    return c;
}

void
printSimptrNotInitWarning(const char* funcname)
{
    py::print("Warn:",
              funcname,
              "simptr is not initialized. set boundaries/dim first.");
}

/**
 * @brief Get the random seed.
 *
 * @return
 */
size_t
getRandomSeed(Simulation& sim)
{
    if (!sim.getSimPtr()) {
        printSimptrNotInitWarning(__FUNCTION__);
        return -1;
    }
    return sim.getSimPtr()->randseed;
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
int
init_and_run(const string& filepath,
             const string& flags,
             bool wflag,
             bool quit_at_end = false)
{
    int er = 0;
    auto p = splitPath(filepath);
    simptr pSim = nullptr;

#ifdef OPTION_VCELL
    er = simInitAndLoad(p.first.c_str(),
                        p.second.c_str(),
                        &pSim,
                        flags.c_str(),
                        new SimpleValueProviderFactory(),
                        new SimpleMesh());
#else
    er =
      simInitAndLoad(p.first.c_str(), p.second.c_str(), &pSim, flags.c_str());
#endif
    if (!er) {
        pSim->quitatend = quit_at_end;
        if (pSim->graphss && pSim->graphss->graphics != 0)
            gl2glutInit(0, nullptr);
        er = simUpdateAndDisplay(pSim);
    }
    if (!er)
        er = smolOpenOutputFiles(pSim, wflag);
    if (er) {
        simLog(pSim, 4, "%sSimulation skipped\n", er ? "\n" : "");
    } else {
        fflush(stdout);
        fflush(stderr);
        if (!pSim->graphss || pSim->graphss->graphics == 0 ||
            strchr(pSim->flags, 't')) {
            er = smolsimulate(pSim);
            endsimulate(pSim, er);
        } else {
            smolsimulategl(pSim);
        }
    }

    if (pSim)
        simfree(pSim);

    return er;
}

/**
 * @brief Definition of Python module _smoldyn.
 *
 * @param _smoldyn (name of the module)
 * @param m
 */
PYBIND11_MODULE(_smoldyn, m)
{
    // py::options options;
    // options.disable_function_signatures();

    m.doc() = R"pbdoc(
        Low level Python interface for the smoldyn simulator. This module is not
        meant for direct user interaction. The user api defined in smoldyn/smoldyn.py
        uses this module to create an user friendly API.
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
      .value("fsoln", MolecState::MSsoln) // fsoln is really soln
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
      .def_property(
        "func", &CallbackFunc::getFuncName, &CallbackFunc::setFuncName)
      .def("evalAndUpdate", &CallbackFunc::evalAndUpdate);

    /* graphics */
    py::class_<graphicssuperstruct>(m, "Graphics")
      .def_readonly("condition",
                    &graphicssuperstruct::condition) // structure condition
      .def_readonly("method",
                    &graphicssuperstruct::graphics) // graphics: 0=none,
                                                    // 1=opengl, 2=good opengl
      .def_readonly("runmode",
                    &graphicssuperstruct::runmode) // 0=Smoldyn, 1=Libsmoldyn
      .def_readonly("currentIter",
                    &graphicssuperstruct::currentit) // current number of
                                                     // simulation time steps
      .def_readonly("graphicIter",
                    &graphicssuperstruct::graphicit) // number of time steps
                                                     // per graphics update
      .def_readonly("graphicDelay",
                    &graphicssuperstruct::graphicdelay) // minimum delay (in ms)
      // for graphics updates
      .def_readonly(
        "tiffIter",
        &graphicssuperstruct::tiffit) // number of time steps per tiff save
      .def_readonly(
        "framePoints",
        &graphicssuperstruct::framepts) // thickness of frame for graphics
      .def_readonly("gridPoints",
                    &graphicssuperstruct::gridpts) // thickness of virtual
                                                   // box grid for graphics
      .def_readonly("frameColor",
                    &graphicssuperstruct::framecolor) // frame color [c]
      .def_property_readonly("gridColor",
                             [](const graphicssuperstruct& st) {
                                 return pycolor(st.gridcolor);
                             }) // grid color [c]
      .def_property_readonly("bgColor",
                             [](const graphicssuperstruct& st) {
                                 return pycolor(st.backcolor);
                             }) // background color [c]
      .def_property_readonly("textColor",
                             [](const graphicssuperstruct& st) {
                                 return pycolor(st.textcolor);
                             }) // text color [c]
      .def_readonly(
        "maxTextItems",
        &graphicssuperstruct::maxtextitems) // allocated size of item list
      .def_readonly(
        "nTextItems",
        &graphicssuperstruct::ntextitems) // actual size of item list
      .def_property_readonly("textItems",
                             [](const graphicssuperstruct& gst) {
                                 vector<std::string> txt(gst.ntextitems, "");
                                 for (int i = 0; i < gst.ntextitems; i++) {
                                     const char* t = gst.textitems[i];
                                     txt[i] = std::string(t);
                                 }
                                 return txt;
                             })
      // .def_readonly(
      //     "roomstate", &graphicssuperstruct::roomstate)  // on, off, or
      //     auto (on)
      .def_property_readonly("globalAmbientLightColor",
                             [](const graphicssuperstruct& st) {
                                 return pycolor(st.ambiroom);
                             }) // global ambient light [c]
      // .def_readonly("lightstate",
      //     &graphicssuperstruct::lightstate)  // on, off, or auto (off) [lt]
      .def_property_readonly("ambientLightColor",
                             [](const graphicssuperstruct& st) {
                                 vector<array<double, 4>> ambient(MAXLIGHTS);
                                 for (size_t i = 0; i < MAXLIGHTS; i++)
                                     ambient[i] = pycolor(st.ambilight[i]);
                                 return ambient;
                             })
      .def_property_readonly("diffuseLightColor",
                             [](const graphicssuperstruct& st) {
                                 vector<array<double, 4>> difflight(MAXLIGHTS);
                                 for (size_t i = 0; i < MAXLIGHTS; i++)
                                     difflight[i] = pycolor(st.difflight[i]);
                                 return difflight;
                             }) // diffuse light color [lt][c]
      .def("specularLightColor",
           [](const graphicssuperstruct& st) {
               vector<array<double, 4>> light(MAXLIGHTS);
               for (size_t i = 0; i < MAXLIGHTS; i++)
                   light[i] = pycolor(st.speclight[i]);
               return light;
           }) // specular light color [lt][c]
      .def("lightPosition",
           [](const graphicssuperstruct& st, size_t lindex) {
               return st.lightpos[lindex];
           }) // light positions [lt][d]
      ;

    /* wallptr */
    py::class_<wallstruct>(m, "wallstruct")
      .def(py::init<>())
      .def_readonly("wdim", &wallstruct::wdim)
      .def_readonly("side", &wallstruct::side)
      .def_readonly("pos", &wallstruct::pos)
      .def_readonly("type", &wallstruct::type)
      .def_readonly("opp", &wallstruct::opp);

    /* simptr */

    // We only expose types for which pybind11 provides C++ -> Python wrappers
    // as as int, float, string etc. If you want ot expose a
    // struct, make sure to create a Python wrapper just like simstruct below.
    py::class_<simstruct>(m, "simstruct")
      .def(py::init<>())
      //.def_readonly("condition", &simstruct::condition, "Structure
      // condition")

      // Python user can write these values.
      //.def_readwrite("logfile", &simstruct::logfile, "configuration file
      // path")
      .def_readwrite(
        "filepath", &simstruct::filepath, "configuration file path")
      .def_readwrite(
        "filename", &simstruct::filename, "configuration file name")
      .def_readwrite(
        "flags", &simstruct::flags, "command-line options from user")
      .def_readwrite(
        "quitatend", &simstruct::quitatend, "simulation quits at the end")

      // These are readonly.
      .def_readonly(
        "clockstt", &simstruct::clockstt, "clock starting time of simulation")
      .def_readonly(
        "elapsedtime", &simstruct::elapsedtime, "elapsed time of simulation")
      .def_readonly(
        "randseed", &simstruct::randseed, "random number generator seed")
      .def_readonly(
        "eventcount", &simstruct::eventcount, "counter for simulation events")
      .def_readonly(
        "maxvar", &simstruct::maxvar, "allocated user-settable variables")
      .def_readonly(
        "nvar", &simstruct::nvar, "number of user-settable variables")
      .def_readonly("dim", &simstruct::dim, "dimensionality of space.")
      .def_readonly(
        "accuracy", &simstruct::accur, "accuracy, on scale from 0 to 10")
      .def_readonly("time", &simstruct::time, "current time in simulation")
      .def_readonly("tmin", &simstruct::tmin, "simulation start time")
      .def_readonly("tmax", &simstruct::tmax, "simulation end time")
      .def_readonly("tbreak", &simstruct::tbreak, "simulation break time")
      .def_readonly("dt", &simstruct::dt, "simulation time step")
      //.def_readonly("rules", &simstruct::ruless, "rule superstructure")
      //.def_readonly("molecuels", &simstruct::mols, "molecule
      // superstructure") .def_readonly("surfaces", &simstruct::srfss,
      // "surface superstructure") .def_readonly("boxs", &simstruct::boxs,
      // "box superstructure") .def_readonly("cmpts", &simstruct::cmptss,
      //"compartment superstructure") .def_readonly("ports",
      //&simstruct::portss, "port superstructure") .def_readonly("lattices",
      //&simstruct::latticess, "lattice superstructure")
      //.def_readonly("bionets", &simstruct::bngss, "bionetget
      // superstructure") .def_readonly("filaments", &simstruct::filss,
      //"filament superstructure") .def_readonly("commands", &simstruct::cmds,
      //"command superstructure") .def_readonly( "graphics",
      //&simstruct::graphss,
      // py::return_value_policy::reference)
      ;

    /**
     * @brief Simulation class. Also see Simulation.h
     *
     * It contains a self-contained model. Each object of this class has its own
     * private simptr. This is an standalone unit of simulation.
     *
     * The most basic members are defined in Simulation.h which is
     * extended here.
     *
     * Also see source/python/smoldyn/smoldym.py file which use this class as
     * a base class. The child class in Python has extensive docstring, user
     * friendly error messages and an object-oriented design. It is possible to
     * build a complete smoldyn model using this module only.
     */
    py::class_<Simulation>(m, "Simulation")
      .def(py::init<vector<double>&, vector<double>&, vector<string>&>())
      .def(py::init<const char*, const char*>())

      // Connect a python callback function.
      .def("connect", &Simulation::connect)

      // utility functions.
      .def("getSimPtr",
           &Simulation::getSimPtr,
           py::return_value_policy::reference_internal)
      .def_property_readonly("simptr",
                             &Simulation::getSimPtr,
                             py::return_value_policy::reference_internal)

      .def("setModelpath", &Simulation::setModelpath)
      .def("getBoundaries", &Simulation::getBoundaries)

      // Simulation extra.
      .def("runSim", &Simulation::runSim)
      .def("runUntil", &Simulation::runUntil)
      .def("updateSim",
           [](const Simulation& sim) { smolUpdateSim(sim.getSimPtr()); })

      /* Data */
      .def("getOutputData",
           [](Simulation& sim, char* dataname, bool erase) {
               int nrow, ncol;
               double* array;

               smolGetOutputData(
                 sim.getSimPtr(), dataname, &nrow, &ncol, &array, erase);
               assert(array);
               std::vector<vector<double>> cppdata(nrow);
               for (int i = 0; i < nrow; i++)
                   cppdata[i] =
                     vector<double>(array + i * ncol, array + (i + 1) * ncol);
               if (array)
                   free(array);
               return cppdata;
           })

      .def("addOutputData",
           [](Simulation& sim, char* dataname) {
               return smolAddOutputData(sim.getSimPtr(), dataname);
           })

      /*************************
       *  Simulation settings  *
       *************************/
      // enum ErrorCode smolSetSimTimes(
      //         simptr sim, double timestart, double timestop, double
      //         timestep);
      .def(
        "setSimTimes",
        [](
          Simulation& sim, double timestart, double timestop, double timestep) {
            return smolSetSimTimes(
              sim.getSimPtr(), timestart, timestop, timestep);
        })

      // get/set start
      .def_property(
        "start",
        [](Simulation& sim) { return sim.getSimPtr()->tmin; },
        [](Simulation& sim, double time) {
            return smolSetTimeStart(sim.getSimPtr(), time);
        })

      //  get/set stop
      .def_property(
        "stop",
        [](Simulation& sim) { return sim.getSimPtr()->tmax; },
        [](Simulation& sim, double timestop) {
            assert(timestop > 0.0);
            return smolSetTimeStop(sim.getSimPtr(), timestop);
        })

      // get/set dt
      .def_property(
        "dt",
        [](Simulation& sim) { return sim.getSimPtr()->dt; },
        [](Simulation& sim, double timestep) {
            assert(timestep > 0.0);
            return smolSetTimeStep(sim.getSimPtr(), timestep);
        })

      /* set accuracy */
      .def_property(
        "accuracy",
        [](Simulation& sim) { return sim.getSimPtr()->accur; },
        [](Simulation& sim, double accuracy) {
            sim.getSimPtr()->accur = accuracy;
        })

      /* set seed */
      .def_property(
        "seed",
        [](Simulation& sim) { return sim.getSimPtr()->randseed; },
        [](Simulation& sim, int seed) {
            smolSetRandomSeed(sim.getSimPtr(), seed);
        })

      /* quit at end */
      .def_property(
        "quitatend",
        [](const Simulation& sim) { return sim.getSimPtr()->quitatend; },
        [](const Simulation& sim, bool quitatend) {
            // py::print("Calling quitatend", quitatend);
            sim.getSimPtr()->quitatend = quitatend;
        })

      // enum ErrorCode smolSetTimeNow(simptr sim, double timenow);
      .def("setTimeNow",
           [](Simulation& sim, double timenow) {
               return smolSetTimeNow(sim.getSimPtr(), timenow);
           })

      .def("setRandomSeed",
           [](Simulation& sim, long int seed) {
               return smolSetRandomSeed(sim.getSimPtr(), seed);
           })

      // enum ErrorCode smolSetPartitions(simptr sim, const char *method,
      // double value);
      .def("setPartitions",
           [](Simulation& sim, const char* method, double value) {
               return smolSetPartitions(sim.getSimPtr(), method, value);
           })

      /*********************************
       *  Graphics related functions.  *
       *********************************/
      // enum ErrorCode smolSetGraphicsParams(simptr sim, const char *method,
      // int timesteps, int delay);
      .def("setGraphicsParams",
           [](Simulation& sim, const char* method, int timestep, int delay) {
               return smolSetGraphicsParams(
                 sim.getSimPtr(), method, timestep, delay);
           })

      // enum ErrorCode smolSetTiffParams(simptr sim, int timesteps,
      //     const char *tiffname, int lowcount, int highcount);
      .def("setTiffParams",
           [](Simulation& sim,
              int timesteps,
              const char* tiffname,
              int lowcount,
              int highcount) {
               return smolSetTiffParams(
                 sim.getSimPtr(), timesteps, tiffname, lowcount, highcount);
           })

      // enum ErrorCode smolSetLightParams(simptr sim, int lightindex, double
      // *ambient,
      //     double *diffuse, double *specular, double *position);
      .def("setLightParams",
           [](Simulation& sim,
              int lightindex,
              vector<double>& ambient,
              vector<double>& diffuse,
              vector<double>& specular,
              vector<double>& position) {
               return smolSetLightParams(sim.getSimPtr(),
                                         lightindex,
                                         &ambient[0],
                                         &diffuse[0],
                                         &specular[0],
                                         &position[0]);
           })

      // enum ErrorCode smolSetBackgroundStyle(simptr sim, double *color);
      .def("setBackgroundStyle",
           [](Simulation& sim, char* color) {
               array<double, 4> rgba = { 0, 0, 0, 1.0 };
               graphicsreadcolor(&color, &rgba[0]);
               // cout << "debug: Setting background color " << rgba[0] << ' '
               // << rgba[1]
               // << ' '
               // << rgba[2] << ' ' << rgba[3] << endl;
               return smolSetBackgroundStyle(sim.getSimPtr(), &rgba[0]);
           })
      .def("setBackgroundStyle",
           [](Simulation& sim, array<double, 4> rgba) {
               return smolSetBackgroundStyle(sim.getSimPtr(), &rgba[0]);
           })

      // enum ErrorCode smolSetFrameStyle(simptr sim, double thickness, double
      // *color);
      .def("setFrameStyle",
           [](Simulation& sim, double thickness, char* color) {
               array<double, 4> rgba = { 0, 0, 0, 1.0 };
               graphicsreadcolor(&color, &rgba[0]);
               return smolSetFrameStyle(sim.getSimPtr(), thickness, &rgba[0]);
           })

      .def("setFrameStyle",
           [](Simulation& sim, double thickness, array<double, 4>& rgba) {
               return smolSetFrameStyle(sim.getSimPtr(), thickness, &rgba[0]);
           })

      // enum ErrorCode smolSetGridStyle(simptr sim, double thickness, double
      // *color);
      .def("setGridStyle",
           [](Simulation& sim, double thickness, char* color) {
               array<double, 4> rgba = { 0, 0, 0, 1.0 };
               graphicsreadcolor(&color, &rgba[0]);
               return smolSetGridStyle(sim.getSimPtr(), thickness, &rgba[0]);
           })

      .def("setGridStyle",
           [](Simulation& sim, double thickness, array<double, 4>& rgba) {
               return smolSetGridStyle(sim.getSimPtr(), thickness, &rgba[0]);
           })

      // enum ErrorCode smolSetTextStyle(simptr sim, double *color);
      .def("setTextStyle",
           [](Simulation& sim, char* color) {
               array<double, 4> rgba = { 0, 0, 0, 1.0 };
               graphicsreadcolor(&color, &rgba[0]);
               return smolSetTextStyle(sim.getSimPtr(), &rgba[0]);
           })
      .def("setTextStyle",
           [](Simulation& sim, array<double, 4>& rgba) {
               return smolSetTextStyle(sim.getSimPtr(), &rgba[0]);
           })

      // enum ErrorCode smolAddTextDisplay(simptr sim, char *item);
      .def("addTextDisplay",
           [](Simulation& sim, char* item) {
               return smolAddTextDisplay(sim.getSimPtr(), item);
           })

      /***********************
       *  Runtime commands.  *
       ***********************/
      // enum ErrorCode smolSetOutputPath(simptr sim, const char *path);
      .def("setOutputPath",
           [](Simulation& sim, const char* path) {
               return smolSetOutputPath(sim.getSimPtr(), path);
           })

      // enum ErrorCode smolAddOutputFile(simptr sim, char *filename, int
      // suffix, int append);
      .def("addOutputFile",
           [](Simulation& sim, char* filename, int suffix, bool append) {
               return smolAddOutputFile(
                 sim.getSimPtr(), filename, suffix, append);
           })

      /****************
       * Commands *
       ****************/
      .def("addCommand",
           [](Simulation& sim,
              const string& cmd,
              const char cmd_type,
              py::kwargs kwargs) {
               map<string, double> options;
               for (auto it = kwargs.begin(); it != kwargs.end(); it++)
                   options[it->first.cast<string>()] =
                     it->second.cast<double>();
               return sim.addCommand(cmd, cmd_type, options);
           })

      /***************
       *  Molecules  *
       ***************/
      // enum ErrorCode smolAddSpecies(simptr sim, const char *species, const
      // char *mollist);
      .def(
        "addSpecies",
        [](Simulation& sim, const char* species, const char* mollist) {
            return smolAddSpecies(sim.getSimPtr(), species, mollist);
        },
        "species"_a,
        "mollist"_a = "")

      // int   smolGetSpeciesIndex(simptr sim, const char *species);
      .def("getSpeciesIndex",
           [](Simulation& sim, const char* species) -> int {
               return smolGetSpeciesIndex(sim.getSimPtr(), species);
           })

      // int   smolGetSpeciesIndexNT(simptr sim, const char *species);
      .def("speciesIndexNT",
           [](Simulation& sim, const char* species) {
               return smolGetSpeciesIndexNT(sim.getSimPtr(), species);
           })

      // char *smolGetSpeciesName(simptr sim, int speciesindex, char
      // *species);
      .def("getSpeciesName",
           [](Simulation& sim, int speciesindex) {
               char species[128];
               smolGetSpeciesName(sim.getSimPtr(), speciesindex, species);
               return std::string(species);
           })

      // enum ErrorCode smolSetSpeciesMobility(simptr sim, const char
      // *species,
      //     enum MolecState state, double difc, double *drift, double
      //     *difmatrix);
      .def(
        "setSpeciesMobility",
        [](Simulation& sim,
           const char* species,
           MolecState state,
           double difc,
           vector<double>& drift,
           vector<double>& difmatrix) {
            return smolSetSpeciesMobility(
              sim.getSimPtr(), species, state, difc, &drift[0], &difmatrix[0]);
        },
        "species"_a,
        "state"_a,
        "diffConst"_a,
        "drift"_a = std::vector<double>(),
        "difmatrix"_a = std::vector<double>())

      //?? needs function smolSetSpeciesSurfaceDrift
      // enum ErrorCode smolAddMolList(simptr sim, const char *mollist);
      .def("addMolList",
           [](Simulation& sim, const char* mollist) {
               return smolAddMolList(sim.getSimPtr(), mollist);
           })

      // int   smolGetMolListIndex(simptr sim, const char *mollist);
      .def("getMolListIndex",
           [](Simulation& sim, const char* mollist) {
               return smolGetMolListIndex(sim.getSimPtr(), mollist);
           })

      // int   smolGetMolListIndexNT(simptr sim, const char *mollist);
      .def("molListIndexNT",
           [](Simulation& sim, const char* mollist) {
               return smolGetMolListIndexNT(sim.getSimPtr(), mollist);
           })

      // char *smolGetMolListName(simptr sim, int mollistindex, char
      // *mollist);
      .def("getMolListName",
           [](Simulation& sim, int mollistindex, char* species) {
               return smolGetMolListName(
                 sim.getSimPtr(), mollistindex, species);
           })

      // enum ErrorCode smolSetMolList(simptr sim, const char *species, enum
      // MolecState state, const char *mollist);
      .def("setMolList",
           [](Simulation& sim,
              const char* species,
              MolecState state,
              const char* mollist) {
               return smolSetMolList(sim.getSimPtr(), species, state, mollist);
           })

      // enum ErrorCode smolSetMaxMolecules(simptr sim, int maxmolecules);
      .def("setMaxMolecules",
           [](Simulation& sim, int maxmolecules) {
               return smolSetMaxMolecules(sim.getSimPtr(), maxmolecules);
           })

      // enum ErrorCode smolAddSolutionMolecules(simptr sim, const char
      // *species,
      //     int number, double *lowposition, double *highposition);
      .def(
        "addSolutionMolecules",
        [](Simulation& sim,
           const char* species,
           size_t number,
           vector<double>& lowposition,
           vector<double>& highposition) {
            return smolAddSolutionMolecules(sim.getSimPtr(),
                                            species,
                                            number,
                                            &lowposition[0],
                                            &highposition[0]);
        },
        "species"_a,
        "number"_a,
        "lowpos"_a = vector<double>(),
        "highpos"_a = vector<double>())

      // enum ErrorCode smolAddCompartmentMolecules(
      //     simptr sim, const char *species, int number, const char
      //     *compartment);
      .def(
        "addCompartmentMolecules",
        [](
          Simulation& sim, char* species, int number, const char* compartment) {
            return smolAddCompartmentMolecules(
              sim.getSimPtr(), species, number, compartment);
        })

      // enum ErrorCode smolAddSurfaceMolecules(simptr sim, const char
      // *species,
      //     enum MolecState state, int number, const char *surface,
      //     enum PanelShape panelshape, const char *panel, double *position);
      .def("addSurfaceMolecules",
           [](Simulation& sim,
              const char* species,
              MolecState state,
              int number,
              const char* surface,
              PanelShape panelshape,
              const char* panel,
              vector<double>& position) {
               return smolAddSurfaceMolecules(sim.getSimPtr(),
                                              species,
                                              state,
                                              number,
                                              surface,
                                              panelshape,
                                              panel,
                                              &position[0]);
           })

      // int smolGetMoleculeCount(simptr sim, const char *species, enum
      // MolecState state);
      .def("getMoleculeCount",
           [](Simulation& sim, const char* species, MolecState state) {
               return smolGetMoleculeCount(sim.getSimPtr(), species, state);
           })

      // enum ErrorCode smolSetMoleculeStyle(simptr sim, const char *species,
      //     enum MolecState state, double size, double *color);
      .def("setMoleculeStyle",
           [](Simulation& sim,
              const char* species,
              MolecState state,
              double size,
              char* color) {
               auto rgba = color2RGBA(color);
               return smolSetMoleculeStyle(
                 sim.getSimPtr(), species, state, size, &rgba[0]);
           })
      .def("setMoleculeStyle",
           [](Simulation& sim,
              const char* species,
              MolecState state,
              double size,
              array<double, 4>& rgba) {
               return smolSetMoleculeStyle(
                 sim.getSimPtr(), species, state, size, &rgba[0]);
           })

      // enum ErrorCode smolSetMoleculeColor(simptr sim, const char *species,
      //     enum MolecState state, double *color);
      .def(
        "setMoleculeColor",
        [](
          Simulation& sim, const char* species, MolecState state, char* color) {
            auto rgba = color2RGBA(color);
            return smolSetMoleculeColor(
              sim.getSimPtr(), species, state, &rgba[0]);
        })

      .def("setMoleculeColor",
           [](Simulation& sim,
              const char* species,
              MolecState state,
              array<double, 4>& rgba) {
               return smolSetMoleculeColor(
                 sim.getSimPtr(), species, state, &rgba[0]);
           })

      // enum ErrorCode smolSetMoleculeSize(simptr sim, const char *species,
      //     enum MolecState state, double size);
      .def(
        "setMoleculeSize",
        [](
          Simulation& sim, const char* species, MolecState state, double size) {
            return smolSetMoleculeSize(sim.getSimPtr(), species, state, size);
        })

      /*************
       *  Surface  *
       *************/
      // enum ErrorCode smolSetBoundaryType(simptr sim, int dimension, int
      // highside, char type);
      .def("setBoundaryType",
           [](Simulation& sim, int dimension, int highside, char type) {
               return smolSetBoundaryType(
                 sim.getSimPtr(), dimension, highside, type);
           })

      // enum ErrorCode smolAddSurface(simptr sim, const char *surface);
      .def("addSurface",
           [](Simulation& sim, const char* surface) {
               return smolAddSurface(sim.getSimPtr(), surface);
           })

      // int molGetSurfaceIndex(simptr sim, const char *surface);
      .def("getSurfaceIndex",
           [](Simulation& sim, const char* surface) {
               return smolGetSurfaceIndex(sim.getSimPtr(), surface);
           })

      // int            smolGetSurfaceIndexNT(simptr sim, const char
      // *surface);
      .def("getSurfaceIndexNT",
           [](Simulation& sim, const char* surface) {
               return smolGetSurfaceIndexNT(sim.getSimPtr(), surface);
           })

      // char *smolGetSurfaceName(simptr sim, int surfaceindex, char
      // *surface);
      .def("getSurfaceName",
           [](Simulation& sim, int surfaceindex, char* surface) {
               return smolGetSurfaceName(
                 sim.getSimPtr(), surfaceindex, surface);
           })

      // enum ErrorCode smolSetSurfaceAction(simptr sim, const char *surface,
      //     enum PanelFace face, const char *species, enum MolecState state,
      //     enum SrfAction action, const char *newspecies);
      .def(
        "setSurfaceAction",
        [](Simulation& sim,
           const char* surface,
           PanelFace face,
           const char* species,
           MolecState state,
           SrfAction action,
           const char* newspecies) {
            return smolSetSurfaceAction(sim.getSimPtr(),
                                        surface,
                                        face,
                                        species,
                                        state,
                                        action,
                                        newspecies);
        },
        "surface"_a,
        "face"_a,
        "species"_a,
        "state"_a,
        "action"_a,
        "newspecies"_a = "")

      // enum ErrorCode smolSetSurfaceRate(simptr sim, const char *surface,
      //     const char *species, enum MolecState state, enum MolecState
      //     state1, enum MolecState state2, double rate, const char
      //     *newspecies, int isinternal);
      .def("setSurfaceRate",
           [](Simulation& sim,
              const char* surface,
              const char* species,
              MolecState state,
              MolecState state1,
              MolecState state2,
              double rate,
              const char* newspecies,
              bool isinternal) {
               return smolSetSurfaceRate(sim.getSimPtr(),
                                         surface,
                                         species,
                                         state,
                                         state1,
                                         state2,
                                         rate,
                                         newspecies,
                                         isinternal);
           })
      // enum ErrorCode smolAddPanel(simptr sim, const char *surface,
      //     enum PanelShape panelshape, const char *panel, const char
      //     *axisstring, double *params);
      .def("addPanel",
           [](Simulation& sim,
              const char* surface,
              PanelShape panelshape,
              const char* panel,
              const char* axisstring,
              vector<double>& params) {
               return smolAddPanel(sim.getSimPtr(),
                                   surface,
                                   panelshape,
                                   panel,
                                   axisstring,
                                   &params[0]);
           })

      // int            smolGetPanelIndex(simptr sim, const char *surface,
      //                enum PanelShape *panelshapeptr, const char *panel);
      .def("getPanelIndex",
           [](Simulation& sim, const char* surface, const char* panel) {
               PanelShape panelshape{ PanelShape::PSnone };
               return smolGetPanelIndex(
                 sim.getSimPtr(), surface, &panelshape, panel);
           })

      // int            smolGetPanelIndexNT(simptr sim, const char *surface,
      //                enum PanelShape *panelshapeptr, const char *panel);
      .def("getPanelIndexNT",
           [](Simulation& sim, const char* surface, const char* panel) {
               PanelShape panelshape{ PanelShape::PSnone };
               return smolGetPanelIndexNT(
                 sim.getSimPtr(), surface, &panelshape, panel);
           })

      // char*  smolGetPanelName(simptr sim, const char *surface, enum
      // PanelShape panelshape, int panelindex, char *panel);
      .def("getPanelName",
           [](Simulation& sim,
              const char* surface,
              PanelShape panelshape,
              int panelindex,
              char* panel) {
               return smolGetPanelName(
                 sim.getSimPtr(), surface, panelshape, panelindex, panel);
           })

      // enum ErrorCode smolSetPanelJump(simptr sim, const char *surface,
      //     const char *panel1, enum PanelFace face1, const char *panel2,
      //     enum PanelFace face2, int isbidirectional);
      .def("setPanelJump",
           [](Simulation& sim,
              const char* surface,
              const char* panel1,
              PanelFace face1,
              const char* panel2,
              PanelFace face2,
              bool isbidirectional) {
               return smolSetPanelJump(sim.getSimPtr(),
                                       surface,
                                       panel1,
                                       face1,
                                       panel2,
                                       face2,
                                       isbidirectional);
           })

      // enum ErrorCode smolAddSurfaceUnboundedEmitter(simptr sim,
      //     const char *surface, enum PanelFace face, const char *species,
      //     double emitamount, double *emitposition);
      .def("addSurfaceUnboundedEmitter",
           [](Simulation& sim,
              const char* surface,
              PanelFace face,
              const char* species,
              double emitamount,
              vector<double>& emitposition) {
               return smolAddSurfaceUnboundedEmitter(sim.getSimPtr(),
                                                     surface,
                                                     face,
                                                     species,
                                                     emitamount,
                                                     &emitposition[0]);
           })

      // enum ErrorCode smolSetSurfaceSimParams(
      //     simptr sim, const char *parameter, double value);
      .def("setSurfaceSimParams",
           [](Simulation& sim, const char* parameter, double value) {
               return smolSetSurfaceSimParams(
                 sim.getSimPtr(), parameter, value);
           })

      // enum ErrorCode smolAddPanelNeighbor(simptr sim, const char *surface1,
      //     const char *panel1, const char *surface2, const char *panel2,
      //     int reciprocal);
      .def(
        "addPanelNeighbor",
        [](Simulation& sim,
           const char* surface1,
           const char* panel1,
           const char* surface2,
           const char* panel2,
           int reciprocal) {
            return smolAddPanelNeighbor(
              sim.getSimPtr(), surface1, panel1, surface2, panel2, reciprocal);
        })

      // enum ErrorCode smolSetSurfaceStyle(simptr sim, const char *surface,
      //     enum PanelFace face, enum DrawMode mode, double thickness,
      //     double *color, int stipplefactor, int stipplepattern, double
      //     shininess);
      .def("setSurfaceStyle",
           [](Simulation& sim,
              const char* surface,
              PanelFace face,
              DrawMode mode,
              double thickness,
              char* color,
              int stipplefactor,
              int stipplepattern,
              double shininess) {
               array<double, 4> rgba = { 0, 0, 0, 1.0 };
               graphicsreadcolor(&color, &rgba[0]);
               return smolSetSurfaceStyle(sim.getSimPtr(),
                                          surface,
                                          face,
                                          mode,
                                          thickness,
                                          &rgba[0],
                                          stipplefactor,
                                          stipplepattern,
                                          shininess);
           })
      .def("setSurfaceStyle",
           [](Simulation& sim,
              const char* surface,
              PanelFace face,
              DrawMode mode,
              double thickness,
              array<double, 4>& rgba,
              int stipplefactor,
              int stipplepattern,
              double shininess) {
               return smolSetSurfaceStyle(sim.getSimPtr(),
                                          surface,
                                          face,
                                          mode,
                                          thickness,
                                          &rgba[0],
                                          stipplefactor,
                                          stipplepattern,
                                          shininess);
           })

      /*****************
       *  Compartment  *
       *****************/
      // enum ErrorCode smolAddCompartment(simptr sim, const char
      // *compartment);
      .def("addCompartment",
           [](Simulation& sim, const char* compartment) {
               return smolAddCompartment(sim.getSimPtr(), compartment);
           })

      // int smolGetCompartmentIndex(simptr sim, const char *compartment);
      .def("getCompartmentIndex",
           [](Simulation& sim, const char* compartment) {
               return smolGetCompartmentIndex(sim.getSimPtr(), compartment);
           })

      // int smolGetCompartmentIndexNT(simptr sim, const char *compartment);
      .def("getCompartmentIndexNT",
           [](Simulation& sim, const char* compartment) {
               return smolGetCompartmentIndexNT(sim.getSimPtr(), compartment);
           })
      // char * smolGetCompartmentName(simptr sim, int compartmentindex, char
      // *compartment);
      .def("getCompartmentName",
           [](Simulation& sim, int compartmentindex, char* compartment) {
               return smolGetCompartmentName(
                 sim.getSimPtr(), compartmentindex, compartment);
           })

      // enum ErrorCode smolAddCompartmentSurface(simptr sim, const char
      // *compartment, const char *surface);
      .def("addCompartmentSurface",
           [](Simulation& sim, const char* compartment, const char* surface) {
               return smolAddCompartmentSurface(
                 sim.getSimPtr(), compartment, surface);
           })

      // enum ErrorCode smolAddCompartmentPoint(simptr sim, const char
      // *compartment, double *point);
      .def("addCompartmentPoint",
           [](Simulation& sim, const char* compartment, vector<double>& point) {
               return smolAddCompartmentPoint(
                 sim.getSimPtr(), compartment, &point[0]);
           })

      // enum ErrorCode smolAddCompartmentLogic(simptr sim, const char
      // *compartment, enum CmptLogic logic, const char *compartment2);
      .def("addCompartmentLogic",
           [](Simulation& sim,
              const char* compartment,
              CmptLogic logic,
              const char* compartment2) {
               return smolAddCompartmentLogic(
                 sim.getSimPtr(), compartment, logic, compartment2);
           })

      /***************
       *  Reactions  *
       ***************/
      // enum ErrorCode smolAddReaction(simptr sim, const char *reaction,
      //         const char *reactant1, enum MolecState rstate1, const char
      //         *reactant2, enum MolecState rstate2, int nproduct, const char
      //         **productspecies, enum MolecState *productstates, double
      //         rate);
      .def("addReaction",
           [](Simulation& sim,
              const char* reaction,              // Name of the reaction.
              const char* reactant1,             // First reactant
              MolecState rstate1,                // First reactant state
              const char* reactant2,             // Second reactant.
              MolecState rstate2,                // second reactant state.
              vector<string>& productSpeciesStr, // product species.
              vector<MolecState>& productStates, // product state.
              double rate                        // rate
           ) {
               // NOTE: Can't use vector<const char*> in the function
               // argument. We'll runinto wchar_t vs char* issue from
               // python2/python3 str/unicode fiasco. Be safe, use string and
               // cast to const char* by ourselves.

               size_t nprd = productStates.size();
               if (nprd >= MAXPRODUCT) {
                   py::print(
                     "Maximum product allowed ", MAXPRODUCT, ". Given ", nprd);
                   return ErrorCode::ECbounds;
               }

               vector<const char*> productSpecies(nprd);
               for (size_t i = 0; i < nprd; i++)
                   productSpecies[i] = productSpeciesStr[i].c_str();

               return smolAddReaction(sim.getSimPtr(),
                                      reaction,
                                      reactant1,
                                      rstate1,
                                      reactant2,
                                      rstate2,
                                      productSpecies.size(),
                                      &productSpecies[0],
                                      &productStates[0],
                                      rate);
           })

      // int smolGetReactionIndex(simptr sim, int *orderptr, const char
      // *reaction);
      .def("getReactionIndex",
           [](Simulation& sim, vector<int>& order, const char* reaction) {
               return smolGetReactionIndex(
                 sim.getSimPtr(), &order[0], reaction);
           })

      // int smolGetReactionIndexNT(simptr sim, int *orderptr, const char
      // *reaction);
      .def("getReactionIndexNT",
           [](Simulation& sim, vector<int>& order, const char* reaction) {
               return smolGetReactionIndexNT(
                 sim.getSimPtr(), &order[0], reaction);
           })

      // char *smolGetReactionName(simptr sim, int order, int reactionindex,
      // char *reaction);
      .def("getReactionName",
           [](Simulation& sim, int order, int reactionindex, char* reaction) {
               return smolGetReactionName(
                 sim.getSimPtr(), order, reactionindex, reaction);
           })

      // enum ErrorCode smolSetReactionRate(simptr sim, const char *reaction,
      // double rate, int type);
      .def("setReactionRate",
           [](Simulation& sim, const char* reaction, double rate, int type) {
               return smolSetReactionRate(
                 sim.getSimPtr(), reaction, rate, type);
           })

      // enum ErrorCode smolSetReactionRegion(simptr sim, const char
      // *reaction, const char *compartment, const char *surface);
      .def("setReactionRegion",
           [](Simulation& sim,
              const char* reaction,
              const char* compartment,
              const char* surface) {
               return smolSetReactionRegion(
                 sim.getSimPtr(), reaction, compartment, surface);
           })

      // enum ErrorCode smolSetReactionProducts(simptr sim, const char
      // *reaction,
      //     enum RevParam method, double parameter, const char *product,
      //     double *position);
      .def("setReactionProducts",
           [](Simulation& sim,
              const char* reaction,
              RevParam method,
              double parameter,
              const char* product,
              vector<double>& position) {
               return smolSetReactionProducts(sim.getSimPtr(),
                                              reaction,
                                              method,
                                              parameter,
                                              product,
                                              &position[0]);
           })

      // enum ErrorCode smolSetReactionIntersurface(simptr sim, const char
      // *reaction,
      //     int* rules);
      .def("setReactionIntersurface",
           [](Simulation& sim, const char* reaction, vector<int>& rules) {
               return smolSetReactionIntersurface(
                 sim.getSimPtr(), reaction, &rules[0]);
           })

      /***********
       *  Ports  *
       ***********/
      // enum ErrorCode smolAddPort(simptr sim, const char *port, const char
      // *surface, enum PanelFace face);
      .def("addPort",
           [](Simulation& sim,
              const char* port,
              const char* surface,
              PanelFace face) {
               return smolAddPort(sim.getSimPtr(), port, surface, face);
           })

      // int smolGetPortIndex(simptr sim, const char *port);
      .def("getPortIndex",
           [](Simulation& sim, const char* port) {
               return smolGetPortIndex(sim.getSimPtr(), port);
           })

      // int smolGetPortIndexNT(simptr sim, const char *port);
      .def("getPortIndexNT",
           [](Simulation& sim, const char* port) {
               return smolGetPortIndexNT(sim.getSimPtr(), port);
           })

      // char * smolGetPortName(simptr sim, int portindex, char *port);
      .def("getPortName",
           [](Simulation& sim, int portindex, char* port) {
               return smolGetPortName(sim.getSimPtr(), portindex, port);
           })

      // enum ErrorCode smolAddPortMolecules(simptr sim, const char *port,
      //     int nmolec, const char *species, double **positions);
      .def("addPortMolecule",
           [](Simulation& sim,
              const char* port,
              int nmolec,
              const char* species,
              vector<vector<double>>& pos) {
               std::vector<double*> ptrs;
               for (auto& vec : pos)
                   ptrs.push_back(vec.data());
               return smolAddPortMolecules(
                 sim.getSimPtr(), port, nmolec, species, ptrs.data());
           })

      // int smolGetPortMolecules(simptr sim, const char *port, const char
      // *species, enum MolecState state, int remove);
      .def("getPortMolecules",
           [](Simulation& sim,
              const char* port,
              const char* species,
              MolecState state,
              bool remove) {
               return smolGetPortMolecules(
                 sim.getSimPtr(), port, species, state, remove);
           })

      /**************
       *  Lattices  *
       **************/
      // enum ErrorCode smolAddLattice(simptr sim, const char *lattice,
      //         const double *min, const double *max, const double *dx, const
      //         char *btype);
      .def("addLattice",
           [](Simulation& sim,
              const char* lattice,
              const vector<double>& min,
              const std::vector<double>& max,
              const vector<double> dx,
              const char* btype) {
               return smolAddLattice(
                 sim.getSimPtr(), lattice, &min[0], &max[0], &dx[0], btype);
           })

      // enum ErrorCode smolAddLatticePort(simptr sim, const char *lattice,
      // const char *port);
      .def("addLatticePort",
           [](Simulation& sim, const char* lattice, const char* port) {
               return smolAddLatticePort(sim.getSimPtr(), lattice, port);
           })

      // enum ErrorCode smolAddLatticeSpecies(simptr sim, const char *lattice,
      // const char *species);
      .def("addLatticeSpecies",
           [](Simulation& sim, const char* lattice, const char* species) {
               return smolAddLatticeSpecies(sim.getSimPtr(), lattice, species);
           })

      // int   smolGetLatticeIndex(simptr sim, const char *lattice);
      .def("getLatticeIndex",
           [](Simulation& sim, const char* lattice) {
               return smolGetLatticeIndex(sim.getSimPtr(), lattice);
           })

      // int   smolGetLatticeIndexNT(simptr sim, const char *lattice);
      .def("getLatticeIndexNT",
           [](Simulation& sim, const char* lattice) {
               return smolGetLatticeIndexNT(sim.getSimPtr(), lattice);
           })

      // char *smolGetLatticeName(simptr sim, int latticeindex, char
      // *lattice);
      .def("getLatticeName",
           [](Simulation& sim, int latticeindex, char* lattice) {
               return smolGetLatticeName(
                 sim.getSimPtr(), latticeindex, lattice);
           })

      // enum ErrorCode smolAddLatticeMolecules(simptr sim, const char
      // *lattice,
      //     const char *species, int number, double *lowposition,
      //     double *highposition);
      .def("addLatticeMolecules",
           [](Simulation& sim,
              const char* lattice,
              const char* species,
              int number,
              vector<double>& lowposition,
              vector<double>& highposition) {
               return smolAddLatticeMolecules(sim.getSimPtr(),
                                              lattice,
                                              species,
                                              number,
                                              &lowposition[0],
                                              &highposition[0]);
           })

      // enum ErrorCode smolAddLatticeReaction(
      //     simptr sim, const char *lattice, const char *reaction, const int
      //     move);
      .def("addLatticeReaction",
           [](Simulation& sim,
              const char* lattice,
              const char* reaction,
              const int move) {
               return smolAddLatticeReaction(
                 sim.getSimPtr(), lattice, reaction, move);
           })

      .def("loadSimFromFile",
           [](Simulation& sim, const string& filepath, const char* flags) {
               auto p = splitPath(filepath);
               simptr psim = sim.getSimPtr();
               return smolLoadSimFromFile(
                 p.first.c_str(), p.second.c_str(), &psim, flags);
           })
      .def("readConfigString",
           [](Simulation& sim, const char* statement, char* params) {
               return smolReadConfigString(sim.getSimPtr(), statement, params);
           })

      /*********************************************************
       * Simulation related                                    *
       *********************************************************/
      .def("runTimeStep",
           [](Simulation& sim) { return smolRunTimeStep(sim.getSimPtr()); })
      .def("runSim",
           [](Simulation& sim) { return smolRunSim(sim.getSimPtr()); })
      .def("runSimUntil",
           [](Simulation& sim, double breaktime, bool overwrite) {
               return smolRunSimUntil(sim.getSimPtr(), breaktime);
           })

      .def("displaySim",
           [](Simulation& sim) { return smolDisplaySim(sim.getSimPtr()); });

    /*******************
     *  Miscellaneous  *
     *******************/
    m.def("getVersion", &smolGetVersion);
    m.def("color2RGBA", &color2RGBA);

    /************
     *  Errors  *
     ************/
    m.def("setDebugMode", &smolSetDebugMode);
    m.def("errorCodeToString",
          [](ErrorCode err) { return smolErrorCodeToString(err, tempstring); });

    /*****************************
     *  Read configuration file  *
     *****************************/
    m.def("prepareSimFromFile", [](const char* filepath, const char* flags) {
        auto path = splitPath(string(filepath));
        return smolPrepareSimFromFile(
          path.first.c_str(), path.second.c_str(), flags);
    });

    /* Function */
    m.def("loadModel",
          &init_and_run,
          "filepath"_a,
          "flags"_a = "",
          "wflag"_a = 0,
          "quit_at_end"_a = 1,
          "Load model from a txt file");

    /* attributes */
    m.attr("__version__") = VERSION; // Version is set by CMAKE
}
