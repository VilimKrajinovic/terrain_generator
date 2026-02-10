#ifndef MESH_H
#define MESH_H

#include "memory/arena.h"
#include "geometry/vertex.h"

// Mesh structure
typedef struct Mesh {
  Vertex *vertices;
  u32     vertex_count;
  u32    *indices;
  u32     index_count;
} Mesh;

// Initialize an empty mesh
void mesh_init(Mesh *mesh);

// Allocate mesh data from arena
bool mesh_allocate(Mesh *mesh, u32 vertex_count, u32 index_count, Arena *arena);

#endif // MESH_H
