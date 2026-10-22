/* Optional, nested wall-clock profiling. Compiled out in ordinary builds. */
#ifndef SMOL_PROFILE_H
#define SMOL_PROFILE_H

enum SmolProfileBucket {
 SP_STEP, SP_FILAMENT, SP_CHEM_PREPARE, SP_TREADMILL, SP_BRANCH,
 SP_CAP, SP_ELONGATE, SP_GROWTH_QUERY, SP_MECHANICS, SP_FORCES,
 SP_STRETCH, SP_BEND, SP_THERMAL, SP_JUNCTION, SP_CONFINE,
 SP_STERIC, SP_INDEX_VALIDATE, SP_INDEX_REBUILD, SP_BOX_MEMBERSHIP,
 SP_MOVE, SP_ROLL_UPDATE, SP_NODES_TO_ANGLES, SP_COMMANDS, SP_OUTPUT,
 SP_COUNT
};

#ifdef SMOLDYN_PROFILE
#ifdef __cplusplus
extern "C" {
#endif
extern int smolProfileActive;
extern int smolProfileDetail;
double smolProfileClock(void);
void smolProfileEnter(enum SmolProfileBucket bucket);
void smolProfileLeave(void);
void smolProfileReset(void);
int smolProfileThreadAllowed(void);
int smolProfileWrite(const char *path);
#ifdef __cplusplus
}
#endif
#define SMOL_PROFILE_RUN(bucket,statement) do { \
 if(smolProfileActive) {smolProfileEnter(bucket);statement;smolProfileLeave();} \
 else {statement;} \
} while(0)
#define SMOL_PROFILE_FINE(bucket,statement) do { \
 if(smolProfileActive && smolProfileDetail && smolProfileThreadAllowed()) {SMOL_PROFILE_RUN(bucket,statement);} \
 else {statement;} \
} while(0)
#else
#define SMOL_PROFILE_RUN(bucket,statement) do {statement;} while(0)
#define SMOL_PROFILE_FINE(bucket,statement) do {statement;} while(0)
#endif
#endif
