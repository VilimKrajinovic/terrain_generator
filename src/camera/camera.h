#ifndef CAMERA_H
#define CAMERA_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "utils/types.h"

enum Movement { FORWARD, BACKWARDS, LEFT, RIGHT };

struct Camera {
  glm::vec3 position{0.f, 0.f, 0.f};
  glm::vec3 front{0.f, 0.f, -1.f};
  glm::vec3 up{0.f, 1.f, 0.f};
  glm::vec3 right{1.f, 0.f, 0.f};
  glm::vec3 world_up{0.f, 1.f, 0.f};
  f32       yaw{-90.f};
  f32       pitch{0.f};
  f32       movement_speed{2.5f};
  f32       mouse_sensitivity{0.1f};
  f32       zoom{45.f};
};

inline void camera_update_vectors(Camera &c) {
  glm::vec3 f;
  f.x     = cos(glm::radians(c.yaw)) * cos(glm::radians(c.pitch));
  f.y     = sin(glm::radians(c.pitch));
  f.z     = sin(glm::radians(c.yaw)) * cos(glm::radians(c.pitch));
  c.front = glm::normalize(f);
  c.right = glm::normalize(glm::cross(c.front, c.world_up));
  c.up    = glm::normalize(glm::cross(c.right, c.front));
}

inline glm::mat4 view_matrix(const Camera &c) { return glm::lookAt(c.position, c.position + c.front, c.up); }

inline Camera camera_default() {
  Camera c{};
  c.position = {0.f, 0.f, 3.f};
  camera_update_vectors(c);
  return c;
}

#endif // !CAMERA_H
