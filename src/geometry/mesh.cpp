#include "mesh.h"
#include "core/log.h"
#include <string.h>

void mesh_init(Mesh *mesh) { memset(mesh, 0, sizeof(Mesh)); }

bool mesh_allocate(Mesh *mesh, u32 vertex_count, u32 index_count, Arena *arena) {
  mesh_init(mesh);

  mesh->vertices = ARENA_PUSH_ARRAY(arena, Vertex, vertex_count);
  if(!mesh->vertices) {
    LOG_ERROR("Failed to allocate vertices");
    return false;
  }

  mesh->indices = ARENA_PUSH_ARRAY(arena, u32, index_count);
  if(!mesh->indices) {
    LOG_ERROR("Failed to allocate indices");
    return false;
  }

  mesh->vertex_count = vertex_count;
  mesh->index_count  = index_count;

  return true;
}
