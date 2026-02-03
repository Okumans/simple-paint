#pragma once

#include <glm/glm.hpp>

struct PointVertex {
  glm::vec2 position;
  glm::vec3 color;
  glm::vec2 uv;
  float total_stroke_length;
};
