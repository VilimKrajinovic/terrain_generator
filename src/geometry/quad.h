#ifndef QUAD_H
#define QUAD_H

#include "geometry/mesh.h"
#include "memory/arena.h"

// Create a colored quad mesh using arena allocation
void quad_create_arena(Mesh *mesh, Arena *arena);

// Create a quad with custom colors using arena allocation
void quad_create_colored_arena(
  Mesh *mesh, const f32 colors[4][3], Arena *arena);

#endif // QUAD_H
