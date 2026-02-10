#include "quad.h"
#include "core/log.h"

void quad_create_arena(Mesh *mesh, Arena *arena) {
  // Default colors: red, green, blue, yellow
  f32 colors[4][3] = {
    {1.0f, 0.0f, 0.0f}, // Red (top-left)
    {0.0f, 1.0f, 0.0f}, // Green (top-right)
    {0.0f, 0.0f, 1.0f}, // Blue (bottom-right)
    {1.0f, 1.0f, 0.0f}, // Yellow (bottom-left)
  };

  quad_create_colored_arena(mesh, colors, arena);
}

void quad_create_colored_arena(Mesh *mesh, const f32 colors[4][3], Arena *arena) {
  LOG_DEBUG("Creating quad mesh");

  // Allocate mesh data (4 vertices, 6 indices)
  if(!mesh_allocate(mesh, 4, 6, arena)) {
    return;
  }

  // Define vertices (centered quad, size ~0.8 for visibility)
  // Top-left
  mesh->vertices[0] = Vertex{
    .position = {-0.5f, -0.5f, 0.0f},
    .color    = {colors[0][0], colors[0][1], colors[0][2]},
  };

  // Top-right
  mesh->vertices[1] = Vertex{
    .position = {0.5f, -0.5f, 0.0f},
    .color    = {colors[1][0], colors[1][1], colors[1][2]},
  };

  // Bottom-right
  mesh->vertices[2] = Vertex{
    .position = {0.5f, 0.5f, 0.0f},
    .color    = {colors[2][0], colors[2][1], colors[2][2]},
  };

  // Bottom-left
  mesh->vertices[3] = Vertex{
    .position = {-0.5f, 0.5f, 0.0f},
    .color    = {colors[3][0], colors[3][1], colors[3][2]},
  };

  // Define indices (two triangles, clockwise winding)
  mesh->indices[0] = 0;
  mesh->indices[1] = 1;
  mesh->indices[2] = 2;

  mesh->indices[3] = 2;
  mesh->indices[4] = 3;
  mesh->indices[5] = 0;

  LOG_DEBUG("Quad mesh created: %u vertices, %u indices", mesh->vertex_count, mesh->index_count);
}
