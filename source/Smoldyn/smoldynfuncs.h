/* Steven Andrews, started 10/22/2001.
 This is the header for the functions in the Smoldyn program.
 See documentation called SmoldynManual.pdf and SmoldynCodeDoc.pdf, and the Smoldyn
 website, which is at www.smoldyn.org.
 Copyright 2003-2016 by Steven Andrews.  This work is distributed under the terms
 of the Gnu Lesser General Public License (LGPL). */

#ifndef __smoldynfuncs_h__
#define __smoldynfuncs_h__

// This is pure C++
#ifdef __cplusplus
#include "smoldyn.h"
#include "SimCommand.h"
extern void (*LoggingCallback)(simptr,int,const char*,...);
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "smoldyn.h"
#include "List.h"
#include "parse.h"

#include "smoldynconfigure.h"			// generated by CMake from smoldynconfigure.h.in


#ifdef OPTION_NSV
	#define NSV_CALL(function) function
#else
	#define NSV_CALL(function) simLog(NULL,11,"ERROR: NextSubvolume option not set. Recompile with OPTION_NSV = ON\n")
#endif

/*********************** Global variable declarations **********************/

extern int ThrowThreshold;
extern FILE* LogFile;

extern char ErrorString[];
extern char ErrorLineAndString[];
extern int ErrorType;
extern char SimFlags[];

#define CHECK(A)				if(!(A)) {ErrorType=1;goto failure;} else (void)0
#define CHECKMEM(A)			if(!(A)) {ErrorType=3;snprintf(ErrorString,STRCHARLONG,"Cannot allocate memory"); goto failure;} else (void)0
#define CHECKS(A,...)		if(!(A)) {ErrorType=2;snprintf(ErrorString,STRCHARLONG,__VA_ARGS__); goto failure;} else (void)0
#define CHECKBUG(A,...)	if(!(A)) {ErrorType=4;snprintf(ErrorString,STRCHARLONG,__VA_ARGS__); goto failure;} else (void)0

extern int VCellDefined;


/********************************* Molecules *******************************/

// enumerated type functions
enum MolecState molstring2ms(char *string);
char *molms2string(enum MolecState ms,char *string);

// low level utilities
char *molserno2string(unsigned long long serno,char *string);
unsigned long long molstring2serno(char *string);
unsigned long long molfindserno(simptr sim,unsigned long long def,long int pserno,unsigned long long r1serno,unsigned long long r2serno,unsigned long long *sernolist);
int molismobile(simptr sim,int species,enum MolecState ms);
int molstring2pattern(const char *str,enum MolecState *msptr,char *pat,int mode);
int molreversepattern(const char *pattern,char *patternrev);
int molpatternindex(simptr sim,const char *pattern,const char *rname,int isrule,int update,int **indexptr);
int molstring2index1(simptr sim,const char *str,enum MolecState *msptr,int **indexptr);
int moladdspeciesgroup(simptr sim,const char *group,char *species,int imol);

// set structure values
int molssetgausstable(simptr sim,int size);
void molsetdifc(simptr sim,int ident,int *index,enum MolecState ms,double difc);
int molsetdifm(simptr sim,int ident,int *index,enum MolecState ms,double *difm);
int molsetdrift(simptr sim,int ident,int *index,enum MolecState ms,double *drift);
int molsetsurfdrift(simptr sim,int ident,int *index,enum MolecState ms,int surface,enum PanelShape ps,double *drift);
void molsetdisplaysize(simptr sim,int ident,int *index,enum MolecState ms,double dsize);
void molsetcolor(simptr sim,int ident,int *index,enum MolecState ms,const double *color);
void molsetlistlookup(simptr sim,int ident,int *index,enum MolecState ms,int ll);
void molsetexist(simptr sim,int ident,enum MolecState ms,int exist);
int molcount(simptr sim,int i,int *index,enum MolecState ms,int max);
void molscancmd(simptr sim,int i,int *index,enum MolecState ms,cmdptr cmd,enum CMDcode(*fn)(simptr,cmdptr,char*));
void molscanfn(simptr sim,int i,int *index,enum MolecState ms,char *erstr,double(*fn)(void*,char*,char*));
int molismatch(moleculeptr mptr,int i,int *index,enum MolecState ms);
double MolCalcDifcSum(simptr sim,int i1,enum MolecState ms1,int i2,enum MolecState ms2);

// memory management
void molssfree(molssptr mols,int maxsrf);
int molexpandsurfdrift(simptr sim,int oldmaxspec,int oldmaxsrf);

// data structure output
void molssoutput(simptr sim);
void writemols(simptr sim,FILE *fptr);
void writemolecules(simptr sim,FILE *fptr);
int checkmolparams(simptr sim,int *warnptr);

// structure setup
int molenablemols(simptr sim,int maxspecies);
void molsetcondition(molssptr mols,enum StructCond cond,int upgrade);
int addmollist(simptr sim,const char *nm,enum MolListType mlt);
int molsetmaxmol(simptr sim,int max);
int moladdspecies(simptr sim,const char *nm);
int molsetexpansionflag(simptr sim,int i,int flag);
int molsupdate(simptr sim);

// adding and removing molecules
void molkill(simptr sim,moleculeptr mptr,int ll,int m);
moleculeptr getnextmol(molssptr mols);
int addmol(simptr sim,int nmol,int ident,double *poslo,double *poshi,int sort);
int addsurfmol(simptr sim,int nmol,int ident,enum MolecState ms,double *pos,panelptr pnl,int surface,enum PanelShape ps,char *pname);
int addcompartmol(simptr sim,int nmol,int ident,compartptr cmpt);

// molecule manipulations
void molchangeident(simptr sim,moleculeptr mptr,int ll,int m,int i,enum MolecState ms,panelptr pnl,double *crsspt);
int molmovemol(simptr sim,moleculeptr mptr,const double *delta);

// core simulation functions
void moldosurfdrift2D(simptr sim,moleculeptr mptr,double dt);
int molsort(simptr sim,int onlydead2live);
int diffuse(simptr sim);

/*********************************** Walls **********************************/

// low level utilities
void systemrandpos(simptr sim,double *pos);
double systemvolume(simptr sim);
void systemcorners(simptr sim,double *poslo,double *poshi);
void systemcenter(simptr sim,double *center);
double systemdiagonal(simptr sim);
int posinsystem(simptr sim,double *pos);
double wallcalcdist2(simptr sim,double *pos1,double *pos2,int wpcode,double *vect);

// memory management
void wallsfree(wallptr *wlist,int dim);

// data structure output
void walloutput(simptr sim);
void writewalls(simptr sim,FILE *fptr);
int checkwallparams(simptr sim,int *warnptr);

// structure setup
int walladd(simptr sim,int d,int highside,double pos,char type);
int wallsettype(simptr sim,int d,int highside,char type);

// core simulation functions
int checkwalls1mol(simptr sim,moleculeptr mptr);
int checkwalls(simptr sim,int ll,int reborn,boxptr bptr);

/********************************* Reactions ********************************/

// enumerated types
enum RevParam rxnstring2rp(const char *string);
char *rxnrp2string(enum RevParam rp,char *string);
enum SpeciesRepresentation rxnstring2sr(const char *string);

// low level utilities
int readrxnname(simptr sim,const char *rname,int *orderptr,rxnptr *rxnpt,listptrv *vlistptr,int rxntype);
int rxnisprod(simptr sim,int i,enum MolecState ms,int code);
long int rxnstring2sernocode(char *pattern,int prd);
char *rxnsernocode2string(long int pserno,char *pattern);

// memory management
void rxnfree(rxnptr rxn);
void rxnssfree(rxnssptr rxnss);
int rxnexpandmaxspecies(simptr sim,int maxspecies);

// data structure output
void rxnoutput(simptr sim,int order);
void writereactions(simptr sim,FILE *fptr);
int checkrxnparams(simptr sim,int *warnptr);

// parameter calculations
int rxnsetrate(simptr sim,int order,int r,char *erstr);

// structure set up
void rxnsetcondition(simptr sim,int order,enum StructCond cond,int upgrade);
int RxnSetValue(simptr sim,const char *option,rxnptr rxn,double value);
int RxnSetValuePattern(simptr sim,const char *option,const char *pattern,const char *rname,const enum MolecState *rctstate,const enum MolecState *prdstate,double value,int oldnresults,const rxnptr templ);
int RxnSetRevparam(simptr sim,rxnptr rxn,enum RevParam rparamt,double rparam,int prd,double *pos,int dim);
void RxnSetPermit(simptr sim,rxnptr rxn,int order,enum MolecState *rctstate,int value);
void RxnSetCmpt(rxnptr rxn,compartptr cmpt);
void RxnSetSurface(rxnptr rxn,surfaceptr srf);
int RxnSetPrdSerno(rxnptr rxn,long int *prdserno);
int RxnSetIntersurfaceRules(rxnptr rxn,int *rules);
int RxnSetRepresentationRules(rxnptr rxn,int order,const enum SpeciesRepresentation *rctrep,const enum SpeciesRepresentation *prdrep);
int RxnSetLog(simptr sim,char *filename,rxnptr rxn,listptrli list,int turnon);
int RxnAddReactionPattern(simptr sim,const char *rname,const char *pattern,int oldnresults,enum MolecState *rctstate,enum MolecState *prdstate,compartptr cmpt,surfaceptr srf,int isrule,rxnptr *rxnpt);
rxnptr RxnAddReaction(simptr sim,const char *rname,int order,int *rctident,enum MolecState *rctstate,int nprod,int *prdident,enum MolecState *prdstate,compartptr cmpt,surfaceptr srf);
rxnptr RxnAddReactionCheck(simptr sim,char *rname,int order,int *rctident,enum MolecState *rctstate,int nprod,int *prdident,enum MolecState *prdstate,compartptr cmpt,surfaceptr srf);
int rxnsupdate(simptr sim);

// reaction parsing
int rxnparsereaction(simptr sim,const char *word,char *line2,char *errstr);

// core simulation functions
int doreact(simptr sim,rxnptr rxn,moleculeptr mptr1,moleculeptr mptr2,int ll1,int m1,int ll2,int m2,double *pos,panelptr rxnpnl);
int zeroreact(simptr sim);
int unireact(simptr sim);
int bireact(simptr sim,int neigh);

/********************************** Rules ***********************************/

// memory management
void rulessfree(rulessptr ruless);

// data structure output
void ruleoutput(simptr sim);
void writerules(simptr sim,FILE *fptr);
int checkruleparams(simptr sim,int *warnptr);

// structure set up
int RuleAddRule(simptr sim,enum RuleType type,const char *rname,const char *pattern,const enum MolecState *rctstate,const enum MolecState *prdstate,double rate,const int *detailsi,const double *detailsf);

// core simulation functions
int RuleExpandRules(simptr sim,int iterations);

/********************************* Surfaces *********************************/

// enumerated types
enum PanelFace surfstring2face(char *string);
char *surfface2string(enum PanelFace face,char *string);
enum PanelShape surfstring2ps(char *string);
char *surfps2string(enum PanelShape ps,char *string);
enum DrawMode surfstring2dm(char *string);
char *surfdm2string(enum DrawMode dm,char *string);
char *surfact2string(enum SrfAction act,char *string);

// low level utilities
int readsurfacename(simptr sim,const char *str,enum PanelShape *psptr,int *pptr);
double panelarea(panelptr pnl,int dim);
double surfacearea(surfaceptr srf,int dim,int *totpanelptr);
double surfacearea2(simptr sim,int surface,enum PanelShape ps,char *pname,int *totpanelptr);
void panelrandpos(panelptr pnl,double *pos,int dim);
panelptr surfrandpos(surfaceptr srf,double *pos,int dim);
int issurfprod(simptr sim,int i,enum MolecState ms);
void srfindex2tristate(enum MolecState ms3,enum PanelFace face,enum MolecState ms4,enum MolecState *msptr,enum MolecState *ms1ptr,enum MolecState *ms2ptr);
int srfcompareaction(enum SrfAction act1,surfactionptr details1,enum SrfAction act2,surfactionptr details2);

// memory management
int surfexpandmollist(surfaceptr srf,int newmax,int ll);
void surfacessfree(surfacessptr srfss);

// data structure output
void surfaceoutput(simptr sim);
void writesurfaces(simptr sim,FILE *fptr);
int checksurfaceparams(simptr sim,int *warnptr);

// structure set up
int surfenablesurfaces(simptr sim,int maxsurf);
int surfexpandmaxspecies(simptr sim,int maxspecies);
surfaceptr surfaddsurface(simptr sim,const char *surfname);
void surfsetcondition(surfacessptr surfss,enum StructCond cond,int upgrade);
int surfsetepsilon(simptr sim,double epsilon);
int surfsetmargin(simptr sim,double margin);
int surfsetneighdist(simptr sim,double neighdist);
int surfsetcolor(surfaceptr srf,enum PanelFace face,double *rgba);
int surfsetedgepts(surfaceptr srf,double value);
int surfsetstipple(surfaceptr srf,int factor,int pattern);
int surfsetdrawmode(surfaceptr srf,enum PanelFace face,enum DrawMode dm);
int surfsetshiny(surfaceptr srf,enum PanelFace face,double shiny);
int surfsetaction(surfaceptr srf,int i,const int *index,enum MolecState ms,enum PanelFace face,enum SrfAction act,int newident);
int surfsetrate(surfaceptr srf,int ident,const int *index,enum MolecState ms,enum MolecState ms1,enum MolecState ms2,int newident,double value,int which);
int surfaddpanel(surfaceptr srf,int dim,enum PanelShape ps,const char *string,double *params,const char *name);
void surftransformpanel(panelptr pnl,int dim,double *translate,double *origin,double *expand); //?? not documented
void surftranslatepanel(panelptr pnl,int dim,double *translate);
void surfupdateoldpos(surfaceptr srf,int dim);
void surftranslatesurf(surfaceptr srf,int dim,double *translate);
int surfsetjumppanel(surfaceptr srf,panelptr pnl1,enum PanelFace face1,int bidirect,panelptr pnl2,enum PanelFace face2);
int surfsetneighbors(panelptr pnl,panelptr *neighlist,int nneigh,int add);
int surfaddemitter(surfaceptr srf,enum PanelFace face,int i,double amount,double *pos,int dim);
surfaceptr surfreadstring(simptr sim,ParseFilePtr pfp,surfaceptr srf,const char *word,char *line2);
int loadsurface(simptr sim,ParseFilePtr *pfpptr,char *line2);
int surfupdate(simptr sim);

// core simulation functions
int surfaddmol(moleculeptr mptr,int ll);
void surfremovemol(moleculeptr mptr,int ll);
enum PanelFace panelside(double* pt,panelptr pnl,int dim,double *distptr,int strict,int useoldpos);
int lineXpanel(double *pt1,double *pt2,panelptr pnl,int dim,double *crsspt,enum PanelFace *face1ptr,enum PanelFace *face2ptr,double *crossptr,double *cross2ptr,int *veryclose,int useoldpos);
int ptinpanel(double *pt,panelptr pnl,int dim);
enum SrfAction surfaction(surfaceptr srf,enum PanelFace face,int ident,enum MolecState ms,int *i2ptr,enum MolecState *ms2ptr);
int rxnXsurface(simptr sim,moleculeptr mptr1,moleculeptr mptr2);
void fixpt2panel(double *pt,panelptr pnl,int dim,enum PanelFace face,double epsilon);
int fixpt2panelnocross(simptr sim,double *pt,panelptr pnl,int dim,enum PanelFace face,double epsilon);
int closestpanelpt(panelptr pnl,int dim,double *testpt,double *pnlpt,double neighdist);
double closestsurfacept(surfaceptr srf,int dim,double *testpt,double *pnlpt,panelptr *pnlptr,boxptr bptr);
void movemol2closepanel(simptr sim,moleculeptr mptr);
int checksurfaces1mol(simptr sim,moleculeptr mptr,double crossminimum);
int checksurfaces(simptr sim,int ll,int reborn);
int checksurfacebound(simptr sim,int ll);

/*********************************** Boxes **********************************/

// low level utilities
boxptr pos2box(simptr sim,const double *pos);
void boxrandpos(simptr sim,double *pos,boxptr bptr);
int boxaddmol(moleculeptr mptr,int ll);
void boxremovemol(moleculeptr mptr,int ll);
boxptr boxscansphere(simptr sim,const double *pos,double radius,boxptr bptr,int *wrap);
int boxdebug(simptr sim);

// memory management
void boxssfree(boxssptr boxs);

// data structure output
void boxoutput(boxssptr boxs,int blo,int bhi,int dim);
void boxssoutput(simptr sim);
int checkboxparams(simptr sim,int *warnptr);

// structure set up
void boxsetcondition(boxssptr boxs,enum StructCond cond,int upgrade);
int boxsetsize(simptr sim,const char *info,double val);
int boxesupdate(simptr sim);

// core simulation functions
boxptr line2nextbox(simptr sim,double *pt1,double *pt2,boxptr bptr);
int reassignmolecs(simptr sim,int diffusing,int reborn);

/******************************* Compartments *******************************/

// enumerated types

// low level utilities
int posincompart(simptr sim,double *pos,compartptr cmpt,int useoldpos);
int compartrandpos(simptr sim,double *pos,compartptr cmpt);
int loadHighResVolumeSamples(simptr sim,ParseFilePtr *pfpptr,char *line2);

// memory management
void compartssfree(compartssptr cmptss);

// data structure output
void compartoutput(simptr sim);
void writecomparts(simptr sim,FILE *fptr);
int checkcompartparams(simptr sim,int *warnptr);

// structure set up
void compartsetcondition(compartssptr cmptss,enum StructCond cond,int upgrade);
int compartenablecomparts(simptr sim,int maxcmpt);
compartptr compartaddcompart(simptr sim,const char *cmptname);
int compartaddsurf(compartptr cmpt,surfaceptr srf);
int compartaddpoint(compartptr cmpt,int dim,double *point);
int compartaddcmptl(compartptr cmpt,compartptr cmptl,enum CmptLogic sym);
compartptr compartreadstring(simptr sim,ParseFilePtr pfp,compartptr cmpt,const char *word,char *line2);
int loadcompart(simptr sim,ParseFilePtr *pfpptr,char *line2);
int compartsupdate(simptr sim);

// core simulation functions
void comparttranslate(simptr sim,compartptr cmpt,int code,double *translate);

/*********************************** Ports **********************************/

// memory management
void portssfree(portssptr portss);

// data structure output
void portoutput(simptr sim);
void writeports(simptr sim,FILE *fptr);
int checkportparams(simptr sim,int *warnptr);

// structure set up
void portsetcondition(portssptr portss,enum StructCond cond,int upgrade);
int portenableports(simptr sim,int maxport);
portptr portaddport(simptr sim,const char *portname,surfaceptr srf,enum PanelFace face);
portptr portreadstring(simptr sim,ParseFilePtr pfp,portptr port,const char *word,char *line2);
int loadport(simptr sim,ParseFilePtr *pfpptr,char* line2);
int portsupdate(simptr sim);

// core simulation functions
int portgetmols(simptr sim,portptr port,int ident,enum MolecState ms,int remove);
int portgetmols2(simptr sim,portptr port,int ident,enum MolecState ms,int remove,double **positions);
int portputmols(simptr sim,portptr port,int nmol,int ident,int *species,double **positions,double **positionsx);
int porttransport(simptr sim1,portptr port1,simptr sim2,portptr port2);

/*********************************** lattices **********************************/

// memory management
void latticessfree(latticessptr latticess);

// data structure output
void latticeoutput(simptr sim);
void writelattices(simptr sim,FILE *fptr);
int checklatticeparams(simptr sim,int *warnptr);

// structure set up
void latticesetcondition(latticessptr latticess,enum StructCond cond,int upgrade);
int loadlattice(simptr sim,ParseFilePtr *pfpptr,char* line2);
int latticesupdate(simptr sim);
int latticeenablelattices(simptr sim);
int latticeaddlattice(simptr sim,latticeptr *latptr,const char *latticename,const double *min,const double *max,const double *dx,const char *btype,enum LatticeType type);
int latticeaddrxn(latticeptr lattice,rxnptr reaction,int move);
int latticeaddsurface(latticeptr lattice, surfaceptr surface);

int latticeaddmols(latticeptr lattice,int nmol,int i,double *poslo,double *poshi,int dim);
void latticeaddport(latticeptr lattice,portptr port);
int latticeaddspecies(latticeptr lattice,int ident,int *index);

// core simulation functions
int latticeruntimestep(simptr sim);

/******************************** Filaments *******************************/

// enumerated types

// low level utilities
int filReadFilName(simptr sim,const char *str,filamenttypeptr *filtypeptr,filamentptr *filptr,char *filname);

// computations on filaments
double *filGetPositionOnFil(filamentptr fil,double frac,double *pos);

// memory management
void filssFree(filamentssptr filss);

// data structure output
void filssOutput(simptr sim);
int filCheckParams(simptr sim,int *warnptr);

// filament manipulation
int filGetFilIndex(simptr sim,const char *name,int *ftptr);
filamentptr filAddFilament(filamenttypeptr filtype,const char *filname);
int filAddRandomSegments(filamentptr fil,int number,const char *xstr,const char *ystr,const char *zstr,const char *thtstr, const char* phistr,const char* chistr,double thickness);

// structure set up
int filEnableFilaments(simptr sim);
filamenttypeptr filtypeReadString(simptr sim,ParseFilePtr pfp,filamenttypeptr filtype,const char *word,char *line2);
filamentptr filReadString(simptr sim,ParseFilePtr pfp,filamentptr fil,filamenttypeptr filtype,const char *word,char *line2);
int filLoadType(simptr sim,ParseFilePtr *pfpptr,char *line2);
int filLoadFil(simptr sim,ParseFilePtr *pfpptr,char *line2);
int filUpdate(simptr sim);

// core simulation functions
void filComputeForces(filamentptr fil,int nodemin,int nodemax);
int filDynamics(simptr sim);

/********************************* BioNetGen ********************************/

// data structure output
void bngoutput(simptr sim);
int checkbngparams(simptr sim,int *warnptr);

// structure setup
int loadbng(simptr sim,ParseFilePtr *pfpptr,char* line2);
int bngupdate(simptr sim);


/********************************* Graphics *********************************/

// enumerated types
enum LightParam graphicsstring2lp(char *string);

// low level utilities
int graphicsreadcolor(char **stringptr,double *rgba);

// memory management
void graphssfree(graphicsssptr graphss);

// data structure output
void graphssoutput(simptr sim);
void writegraphss(simptr sim,FILE *fptr);
int checkgraphicsparams(simptr sim,int *warnptr);

// structure setup
void graphicssetcondition(graphicsssptr graphss,enum StructCond cond,int upgrade);
int graphicsenablegraphics(simptr sim,const char *type);
int graphicssetiter(simptr sim,int iter);
int graphicssettiffiter(simptr sim,int iter);
int graphicssetdelay(simptr sim,int delay);
int graphicssetframethickness(simptr sim,double thickness);
int graphicssetframecolor(simptr sim,double *color);
int graphicssetgridthickness(simptr sim,double thickness);
int graphicssetgridcolor(simptr sim,double *color);
int graphicssetbackcolor(simptr sim,double *color);
int graphicssettextcolor(simptr sim,double *color);
int graphicssettextitem(simptr sim,char *itemname);
int graphicssetlight(simptr sim,graphicsssptr graphss,int lt,enum LightParam ltparam,double *value);

// structure update functions
int graphicsupdate(simptr sim);

// core simulation functions
void smolPostRedisplay(void);

// top level OpenGL functions
void smolsimulategl(simptr sim);

/********************************* Commands *********************************/

enum CMDcode docommand(void *cmdfnarg,cmdptr cmd,char *line);
int loadsmolfunctions(simptr sim);

/******************************** Simulation ********************************/

// error handling
void simSetLogging(simptr sim,const char *logfile,void (*logFunction)(simptr,int,const char*, ...));
void simSetThrowing(int corethreshold);
void simLog(simptr sim,int importance,const char* format, ...);
void simParseError(simptr sim,ParseFilePtr pfp);

// enumerated types
enum SmolStruct simstring2ss(char *string);char *simsc2string(enum StructCond sc,char *string);

// low level utilities
double simversionnumber(void);
void Simsetrandseed(simptr sim,long int randseed);

// memory management
simptr simalloc(const char *root);
void simfree(simptr sim);
void simfuncfree(void);

// data structure output
void simoutput(simptr sim);
void simsystemoutput(simptr sim);
void writesim(simptr sim,FILE *fptr);
int checksimparams(simptr sim);

// structure set up
void simsetcondition(simptr sim,enum StructCond cond,int upgrade);
int simsetvariable(simptr sim,const char *name,double value);
int simsetdim(simptr sim,int dim);
int simsettime(simptr sim,double time,int code);
int simreadstring(simptr sim,ParseFilePtr pfp,const char *word,char *line2);
int loadsim(simptr sim,const char *fileroot,const char *filename,const char *flags);
int simupdate(simptr sim);
#ifdef OPTION_VCELL
	int simInitAndLoad(const char *fileroot,const char *filename,simptr *smptr,const char *flags, ValueProviderFactory* valueProviderFactory, AbstractMesh* mesh);
#else
	int simInitAndLoad(const char *fileroot,const char *filename,simptr *smptr,const char *flags,const char *logfile);
#endif
int simUpdateAndDisplay(simptr sim);

// core simulation functions
void debugcode(simptr sim,const char *prefix);
int simdocommands(simptr sim);
int simulatetimestep(simptr sim);
void endsimulate(simptr sim,int er);
int smolsimulate(simptr sim);

/********************************* Hybrid **********************************/
double evaluateVolRnxRate(simptr sim, rxnptr reaction,  double* pos);
double evaluateMemRnxRate(simptr sim, rxnptr reaction, double* pos, char* panelName);
double evaluateSurfActionRate(simptr sim, surfactionptr actdetails,enum MolecState ms, double* pos, char* panelName);
int randomPosInMesh(simptr sim, double* centerPos,  double* pos);

#ifdef __cplusplus
}
#endif

#endif

