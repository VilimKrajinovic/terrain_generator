#ifndef ENTITY_H
#define ENTITY_H

#include "geometry/mesh.h"
#include "utils/types.h"
#include "glm/glm.hpp"

typedef u32 MeshHandle;

struct Entity {
  glm::vec3  position{0.0f, 0.0f, 0.0f};
  glm::vec3  scale{1.0f, 1.0f, 1.0f};
  Mesh      *mesh;
  MeshHandle mesh_handle;
};

#endif // ENTITY_H
