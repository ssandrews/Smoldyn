/* Profiling only: no random draws, simulation parameters or physics changes. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smolprofile.h"
#ifdef _OPENMP
#include <omp.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

int smolProfileActive=0;
int smolProfileDetail=1;
/* Fine scopes are serial-only; outer phase scopes include the whole team. */
int smolProfileThreadAllowed(void) {
#ifdef _OPENMP
 return !omp_in_parallel();
#else
 return 1;
#endif
}
static double inclusive[SP_COUNT],exclusive[SP_COUNT];
static unsigned long long calls[SP_COUNT];
static struct {enum SmolProfileBucket bucket;double start,children;} stack[32];
static int depth;
static const char *names[SP_COUNT]={
 "timestep","filaments","chemistry_prepare","treadmilling","branching",
 "capping","elongation","growth_collision_query","mechanics","ordinary_forces",
 "stretch_forces","bend_forces","thermal_forces","junction_forces","confinement_forces",
 "steric_forces","index_validation","index_rebuild","box_membership",
 "movement","roll_update","nodes_to_angles","commands","frame_output"
};

double smolProfileClock(void) {
#ifdef _WIN32
 LARGE_INTEGER now;static LARGE_INTEGER frequency;
 if(!frequency.QuadPart) QueryPerformanceFrequency(&frequency);
 QueryPerformanceCounter(&now);return (double)now.QuadPart/frequency.QuadPart;
#else
 struct timespec now;clock_gettime(CLOCK_MONOTONIC,&now);
 return now.tv_sec+now.tv_nsec*1e-9;
#endif
}
void smolProfileEnter(enum SmolProfileBucket bucket) {
 if(depth>=32) {fprintf(stderr,"Profiler nesting overflow\n");abort();}
 stack[depth].bucket=bucket;stack[depth].children=0;
 stack[depth++].start=smolProfileClock();
}
void smolProfileLeave(void) {
 double elapsed;enum SmolProfileBucket bucket;
 if(!depth) return;
 depth--;elapsed=smolProfileClock()-stack[depth].start;bucket=stack[depth].bucket;
 inclusive[bucket]+=elapsed;exclusive[bucket]+=elapsed-stack[depth].children;calls[bucket]++;
 if(depth) stack[depth-1].children+=elapsed;
}
void smolProfileReset(void) {
 memset(inclusive,0,sizeof(inclusive));memset(exclusive,0,sizeof(exclusive));memset(calls,0,sizeof(calls));depth=0;
}
int smolProfileWrite(const char *path) {
 int i;FILE *file=fopen(path,"w");if(!file) return 1;
 fprintf(file,"category,calls,inclusive_seconds,exclusive_seconds\n");
 for(i=0;i<SP_COUNT;i++) fprintf(file,"%s,%llu,%.9f,%.9f\n",names[i],calls[i],inclusive[i],exclusive[i]);
 return fclose(file)!=0;
}
