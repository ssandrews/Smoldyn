/* Fixture adapters: old tests express a branch by its preceding segment;
   production uses upstream node-based junctions and m/p ends exclusively. */
#ifndef FILAMENT_TEST_COMPAT_H
#define FILAMENT_TEST_COMPAT_H
filamentptr filAddBranch(simptr sim,filamentptr mother,int node,char end,double length,const double *angle,double thick,const char *name);
double filBranchAzimuth(const filjunctionptr joint);
void filRemoveJunction(filjunctionptr joint);
static inline filamentptr filTestAddBranch(simptr sim,filamentptr mother,int segment,const double *angle,double thick,const char *name) {
 return filAddBranch(sim,mother,segment+1,'m',0,angle,thick,name);
}
static inline double filTestBranchAzimuth(const filamentptr mother,int segment,const filamentptr daughter) {
 (void)mother;(void)segment;return filBranchAzimuth(daughter->endjunct[0]);
}
#define filAddBranch filTestAddBranch
#define filBranchAzimuth filTestBranchAzimuth
#endif
