#ifndef RGSP_ENERGY_DISTRIBUTION_H
#define RGSP_ENERGY_DISTRIBUTION_H
#include <stdbool.h>
typedef struct { int lasers,shields; } EnergyDistribution;
static inline int EnergyLevel(int level) { return level<0 ? 0 : level>4 ? 4 : level; }
/* Original games give engines the power left after charging systems.
 * More engines reduces both charge allocations one step without wrapping.
 * A shieldless craft must never gain a fictitious shield allocation. */
static inline EnergyDistribution EnergyRedistribute(int lasers,int shields,bool has_shields,bool more_engines) {
 int delta=more_engines ? -1 : 1;
 EnergyDistribution next={EnergyLevel(EnergyLevel(lasers)+delta),EnergyLevel(shields)};
 if(more_engines || has_shields) next.shields=EnergyLevel(next.shields+delta);
 return next;
}
/* Engine presets redistribute laser/shield charging power. */
static inline EnergyDistribution EnergyEnginePreset(int level,bool has_shields) {
 int charge=4-EnergyLevel(level);
 EnergyDistribution next={charge,has_shields ? charge : 0};
 return next;
}
#endif
