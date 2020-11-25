//#include <libsmoldyn.h>

#include "libsmoldyn.h"

/**** Compiling and linking, for Macs ****

Compile source code to object code with either of:
g++ -Wall -O0 -g -c test1.c
gcc -Wall -O0 -g -c test1.c

Static link, if Libsmoldyn was compiled with OpenGL:
(gcc cannot be used for linking because libsmoldyn is in C++ and is a static
library) g++ test1.o -L/usr/local/lib
-I/System/Library/Frameworks/OpenGL.framework/Headers
-I/System/Library/Frameworks/GLUT.framework/Headers -framework GLUT -framework
OpenGL -framework Cocoa -L/System/Library/Frameworks/OpenGL.framework/Libraries
-o test1 -lsmoldyn_static -ltiff

Dynamic link, if Libsmoldyn was compiled with OpenGL, with either of:
gcc test1.o -L/usr/local/lib
-I/System/Library/Frameworks/OpenGL.framework/Headers
-I/System/Library/Frameworks/GLUT.framework/Headers -framework GLUT -framework
OpenGL -framework Cocoa -L/System/Library/Frameworks/OpenGL.framework/Libraries
-o test1 -lsmoldyn_shared -ltiff g++ test1.o -L/usr/local/lib
-I/System/Library/Frameworks/OpenGL.framework/Headers
-I/System/Library/Frameworks/GLUT.framework/Headers -framework GLUT -framework
OpenGL -framework Cocoa -L/System/Library/Frameworks/OpenGL.framework/Libraries
-o test1 -lsmoldyn_shared -ltiff
*/

int main(int argc, char* argv[])
{
    simptr          sim;
    double          lbounds[2];
    double          hbounds[2];
    double          params[10];
    const char*     species[1] = {"ATP"};
    enum MolecState states[1]  = {MSsoln};

    smolSetDebugMode(1);

    lbounds[0] = -50.0;
    lbounds[1] = -50.0;
    hbounds[0] = 50.0;
    hbounds[1] = 50.0;

    sim = smolNewSim(2, lbounds, hbounds);

    smolSetRandomSeed(sim, 1);
    smolSetSimTimes(sim, 0.0, 250.0, 0.01);

    smolSetPartitions(sim, "molperbox", 4);
    smolSetPartitions(sim, "boxsize", 12.5);

    smolAddSpecies(sim, "ACA", "");
    smolAddSpecies(sim, "ATP", "");
    smolAddSpecies(sim, "cAMP", "");
    smolAddSpecies(sim, "cAR1", "");

    smolSetSpeciesMobility(sim, "ACA", MSall, 1, 0, 0);
    smolSetSpeciesMobility(sim, "ATP", MSall, 1, 0, 0);
    smolSetSpeciesMobility(sim, "cAMP", MSall, 1, 0, 0);
    smolSetSpeciesMobility(sim, "cAR1", MSall, 1, 0, 0);

    smolAddSurface(sim, "Membrane00");
    smolSetSurfaceAction(sim, "Membrane00", PFboth, "ATP", MSsoln, SAreflect, NULL);
    params[0] = -20.0;
    params[1] = 20.0;
    params[2] = 10.0;
    params[3] = 20.0;
    params[4] = 20.0;
    smolAddPanel(sim, "Membrane00", PSsph, "", "", params);

    smolAddCompartment(sim, "Cell00");
    smolAddCompartmentSurface(sim, "Cell00", "Membrane00");
    smolAddCompartmentPoint(sim, "Cell00", params);

    smolAddSurfaceMolecules(
        sim, "ACA", MSdown, 30, "Membrane00", PSall, "all", 0);
    smolAddSurfaceMolecules(
        sim, "cAR1", MSup, 30, "Membrane00", PSall, "all", 0);

    smolAddReaction(
        sim, "r100", "", MSall, "", MSall, 1, species, states, 0.02);
    smolSetReactionRegion(sim, "r100", "Cell00", "");

    smolUpdateSim(sim);
    smolDisplaySim(sim);
    smolRunSim(sim);
    smolFreeSim(sim);
    return 0;
}

