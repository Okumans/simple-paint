#pragma once
#include "geometry.h"
#include "glm/fwd.hpp"
#include "ishape.h"
#include <glad/gl.h>

#include <vector>

class Stroke : public IShape {
private:
  std::vector<glm::dvec2> m_raw_points;
  std::vector<PointVertex> m_render_vertices;
  GLuint m_vbo;
  glm::vec3 m_color;
  double m_cummulative_distance;
  double m_thickness;
  AABB m_bounds;
  bool m_is_eraser = false;

public:
  Stroke();
  Stroke(glm::vec3 color, double thickness, bool is_eraser = false);
  ~Stroke();

  // Disable Copying (prevents double-free of VBO)
  Stroke(const Stroke &) = delete;
  Stroke &operator=(const Stroke &) = delete;

  // Enable Moving (transfers ownership of VBO)
  Stroke(Stroke &&other) noexcept;
  Stroke &operator=(Stroke &&other) noexcept;

  void upload() override;
  void draw(GLuint &vao, const Shader &shader) const override;
  void update_geometry() override;
  const AABB &get_bounds() const { return m_bounds; }

  void set_color(glm::vec3 color);
  void set_thickness(double thickness);
  void set_eraser(bool is_eraser) { m_is_eraser = is_eraser; }

  bool is_eraser() const { return m_is_eraser; }
  glm::vec3 get_color() const;
  double get_thickness() const;

  const std::vector<glm::dvec2> &get_raw_points() const;
  void add_point(double x, double y);
  void clear();
  bool is_empty() const;
};
