#include "simulation.h"

#include "core/log.h"
#include "utils/macros.h"

Result simulation_init(SimulationState *simulation, Arena *arena) {
  if(!simulation) {
    return RESULT_ERROR_GENERIC;
  }

  UNUSED(arena);

  simulation->initialized = true;
  LOG_INFO("Simulation initialized");
  return RESULT_SUCCESS;
}

void simulation_shutdown(SimulationState *simulation) {
  if(!simulation || !simulation->initialized) {
    return;
  }

  simulation->initialized = false;
  LOG_INFO("Simulation shutdown");
}

void simulation_update(SimulationState *simulation, f64 delta_time) {
  if(!simulation || !simulation->initialized) {
    return;
  }

  UNUSED(delta_time);
}
