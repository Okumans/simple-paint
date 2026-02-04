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

struct AABB {
  glm::dvec2 min;
  glm::dvec2 max;

  bool intersects(const AABB &other) const {
    return (min.x <= other.max.x && max.x >= other.min.x) &&
           (min.y <= other.max.y && max.y >= other.min.y);
  }
};

void draw_quad();
