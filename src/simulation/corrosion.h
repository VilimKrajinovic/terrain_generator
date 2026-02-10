#ifndef CORROSION_H
#define CORROSION_H

#include "utils/types.h"

// Placeholder for future corrosion simulation
// This will simulate material degradation over time

typedef struct CorrosionParams {
  f32 rate;         // Corrosion rate
  f32 humidity;     // Environmental humidity
  f32 temperature;  // Temperature factor
  f32 salt_content; // Salt concentration
} CorrosionParams;

typedef struct CorrosionState {
  f32            *degradation; // Per-vertex degradation values
  u32             vertex_count;
  CorrosionParams params;
} CorrosionState;

#endif // CORROSION_H
