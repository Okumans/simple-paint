#pragma once
#include "geometry.h"
#include "glm/ext/vector_float2.hpp"
#include "glm/fwd.hpp"
#include "ishape.h"
#include <glad/gl.h>

#include <vector>

class Stroke : public IShape {
private:
  std::vector<glm::vec2> m_raw_points;
  std::vector<PointVertex> m_render_vertices;
  GLuint m_vbo;
  glm::vec3 m_color;
  double m_cummulative_distance;
  double m_thickness;

public:
  Stroke();
  Stroke(glm::vec3 color, double thickness);
  ~Stroke();

  // Disable Copying (prevents double-free of VBO)
  Stroke(const Stroke &) = delete;
  Stroke &operator=(const Stroke &) = delete;

  // Enable Moving (transfers ownership of VBO)
  Stroke(Stroke &&other) noexcept;
  Stroke &operator=(Stroke &&other) noexcept;

  void upload() override;
  void draw(GLuint vao, const Shader &shader) const override;
  void update_geometry() override;

  void set_color(glm::vec3 color);
  void set_thickness(double thickness);
  void add_point(float x, float y);
  void clear();
  bool is_empty() const;
};
