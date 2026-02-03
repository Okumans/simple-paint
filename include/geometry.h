#pragma once

#include <glm/glm.hpp>

struct PointVertex {
  glm::vec2 position;
  glm::vec3 color;
  glm::vec2 uv;
  float thickness;
  float total_stroke_length;
};

struct QuadVertex {
  glm::vec2 pos;
  glm::vec2 uv;
};

void draw_quad();
