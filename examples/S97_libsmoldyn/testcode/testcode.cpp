/* Steve Andrews, 3/16/2011 */

/**** Compiling and linking ****

Compile source code to object code with:
gcc -Wall -O0 -g -c testcode.c

Static link, if Libsmoldyn was compiled without OpenGL:
gcc testcode.o /usr/local/lib/libsmoldyn_static.a -o testcode

Dynamic link, if Libsmoldyn was compiled without OpenGL:
gcc testcode.o -o testcode -lsmoldyn_shared

Static link, if Libsmoldyn was compiled with OpenGL:
gcc testcode.o /usr/local/lib/libsmoldyn_static.a -I/System/Library/Frameworks/OpenGL.framework/Headers -I/System/Library/Frameworks/GLUT.framework/Headers -framework GLUT -framework OpenGL -framework Cocoa -L/System/Library/Frameworks/OpenGL.framework/Libraries -o testcode -ltiff

Dynamic link, if Libsmoldyn was compiled with OpenGL:
gcc testcode.o -I/System/Library/Frameworks/OpenGL.framework/Headers -I/System/Library/Frameworks/GLUT.framework/Headers -framework GLUT -framework OpenGL -framework Cocoa -L/System/Library/Frameworks/OpenGL.framework/Libraries -o testcode -lsmoldyn_shared -ltiff
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libsmoldyn.h>			// This line is required for accessing Libsmoldyn

int main() {
	simptr sim;
	double v1[3],v2[3],color[4],t;
	char **products;
	enum MolecState prodstates[2];
	int ctr,ctf;

	sim=NULL;
	printf("Test program.\n");

	products=(char**) calloc(2,sizeof(char*));
	products[0]=(char*) calloc(256,sizeof(char));
	products[1]=(char*) calloc(256,sizeof(char));

	smolSetDebugMode(1);
	v1[0]=-100;
	v1[1]=-100;
	v1[2]=-10;
	v2[0]=100;
	v2[1]=100;
	v2[2]=10;
	sim=smolNewSim(3,v1,v2);
	smolSetSimTimes(sim,0,2,0.001);
	smolSetBoundaryType(sim,-1,-1,'p');
	smolAddMolList(sim,"rlist");
	smolAddMolList(sim,"flist");
	smolAddSpecies(sim,"rabbit","rlist");
	smolAddSpecies(sim,"fox","flist");
	smolSetSpeciesMobility(sim,"all",MSall,100,NULL,NULL);
	strcpy(products[0],"rabbit");
	strcpy(products[1],"rabbit");
	prodstates[0]=MSsoln;
	prodstates[1]=MSsoln;
	smolAddReaction(sim,"r1","rabbit",MSsoln,NULL,MSnone,2,(const char**) products,prodstates,10);
	strcpy(products[0],"fox");
	strcpy(products[1],"fox");
	smolAddReaction(sim,"r2","rabbit",MSsoln,"fox",MSsoln,2,(const char**) products,prodstates,8000);
	smolAddReaction(sim,"r3","fox",MSsoln,NULL,MSnone,0,NULL,NULL,10);
	smolAddSolutionMolecules(sim,"rabbit",1000,v1,v2);
	smolAddSolutionMolecules(sim,"fox",1000,v1,v2);

	smolSetGraphicsParams(sim,"opengl",5,0);
	color[0]=1;
	color[1]=0;
	color[2]=0;
	color[3]=1;
	smolSetMoleculeStyle(sim,"rabbit",MSall,2,color);
	color[0]=0;
	color[1]=1;
	color[2]=0;
	smolSetMoleculeStyle(sim,"fox",MSall,3,color);
	smolUpdateSim(sim);
	smolDisplaySim(sim);
//	smolRunSim(sim);

	for(t=0.1;t<2;t+=0.1) {
		smolRunSimUntil(sim,t);
		ctr=smolGetMoleculeCount(sim,"rabbit",MSall);
		ctf=smolGetMoleculeCount(sim,"fox",MSall);
		printf("t= %g, rabbit = %i, fox=%i\n",t,ctr,ctf); }

	return 0; }

