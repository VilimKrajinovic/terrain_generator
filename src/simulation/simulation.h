#ifndef SIMULATION_H
#define SIMULATION_H

#include "foundation/result.h"
#include "memory/arena.h"
#include "utils/types.h"

typedef struct SimulationState {
  bool initialized;
} SimulationState;

Result simulation_init(SimulationState *simulation, Arena *arena);
void   simulation_shutdown(SimulationState *simulation);
void   simulation_update(SimulationState *simulation, f64 delta_time);

#endif // SIMULATION_H
