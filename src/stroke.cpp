#include "stroke.h"
#include "geometry.h"
#include "glad/gl.h"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <vector>

Stroke::Stroke()
    : m_vbo(0), m_color(1.0f), m_thickness(0.01), m_cummulative_distance(0.0),
      m_bounds({{0.0f, 0.0f}, {0.0f, 0.0f}}), m_is_eraser(false) {
  m_raw_points.reserve(100);
  m_render_vertices.reserve(100);
}

Stroke::Stroke(glm::vec3 color, double thickness, bool is_eraser)
    : m_vbo(0), m_color(color), m_thickness(thickness),
      m_cummulative_distance(0.0), m_bounds({{0.0f, 0.0f}, {0.0f, 0.0f}}),
      m_is_eraser(is_eraser) {
  m_raw_points.reserve(100);
  m_render_vertices.reserve(100);
}

Stroke::~Stroke() {
  if (m_vbo != 0) {

    std::cout << "DELETING VBO: " << m_vbo << std::endl;
    glDeleteBuffers(1, &m_vbo);
  }
}

Stroke::Stroke(Stroke &&other) noexcept
    : m_raw_points(std::move(other.m_raw_points)),
      m_render_vertices(std::move(other.m_render_vertices)), m_vbo(other.m_vbo),
      m_color(other.m_color), m_bounds(other.m_bounds),
      m_is_eraser(other.m_is_eraser), m_thickness(other.m_thickness) {
  other.m_vbo = 0;
}

Stroke &Stroke::operator=(Stroke &&other) noexcept {
  if (this != &other) {
    if (m_vbo != 0) {
      glDeleteBuffers(1, &m_vbo);
    }

    m_raw_points = std::move(other.m_raw_points);
    m_render_vertices = std::move(other.m_render_vertices);
    m_vbo = other.m_vbo;
    m_color = other.m_color;
    m_bounds = other.m_bounds;
    m_is_eraser = other.m_is_eraser;
    m_thickness = other.m_thickness;

    other.m_vbo = 0;
  }
  return *this;
}

void Stroke::add_point(double x, double y) {
  glm::dvec2 curr_point(x, y);

  if (m_raw_points.empty()) {
    m_raw_points.push_back(curr_point);
    m_render_vertices.push_back({{static_cast<float>(x), static_cast<float>(y)},
                                 m_color,
                                 {0.0f, 0.0f},
                                 static_cast<float>(m_thickness),
                                 0.0f});
    return;
  }

  // Safety: If the mouse didn't move, don't waste memory
  if (m_raw_points.size() >= 2) {
    if (glm::distance(curr_point, m_raw_points.back()) < (m_thickness / 10))
      return;
  }

  m_raw_points.push_back(curr_point);
  if (m_raw_points.size() < 2)
    return;

  glm::dvec2 prev_point = m_raw_points.at(m_raw_points.size() - 2);

  // 1. Update distance
  m_cummulative_distance += glm::distance(curr_point, prev_point);

  // 2. Math for the ribbon
  glm::dvec2 tangent = glm::normalize(curr_point - prev_point);
  glm::dvec2 normal = glm::dvec2(-tangent.y, tangent.x);
  double w = static_cast<double>(m_thickness / 2.0);

  // 3. Handle the very first segment
  if (m_render_vertices.empty()) {
    // We need to place the "Start" vertices at prev_point (v=0)
    glm::dvec2 start_left = prev_point + (normal * w);
    glm::dvec2 start_right = prev_point - (normal * w);

    m_render_vertices.push_back(
        {{static_cast<float>(start_left.x), static_cast<float>(start_left.y)},
         m_color,
         {0.0f, 0.0f},
         static_cast<float>(m_thickness),
         0.0f});
    m_render_vertices.push_back(
        {{static_cast<float>(start_right.x), static_cast<float>(start_right.y)},
         m_color,
         {1.0f, 0.0f},
         static_cast<float>(m_thickness),
         0.0f});
  }

  // 4. Add the "End" vertices for this specific segment
  glm::dvec2 end_left = curr_point + (normal * w);
  glm::dvec2 end_right = curr_point - (normal * w);

  m_render_vertices.push_back(
      {{static_cast<float>(end_left.x), static_cast<float>(end_left.y)},
       m_color,
       {0.0f, m_cummulative_distance},
       static_cast<float>(m_thickness),
       static_cast<float>(m_cummulative_distance)});
  m_render_vertices.push_back(
      {{static_cast<float>(end_right.x), static_cast<float>(end_right.y)},
       m_color,
       {1.0f, m_cummulative_distance},
       static_cast<float>(m_thickness),
       static_cast<float>(m_cummulative_distance)});
}

void Stroke::clear() {
  m_raw_points.clear();
  m_render_vertices.clear();
}

void Stroke::update_geometry() {
  if (m_raw_points.size() < 2)
    return;

  // 1. Path Smoothing (Chaikin's Algorithm)
  // We create a smoother version of the raw input
  std::vector<glm::dvec2> smooth_points = m_raw_points;
  int iterations = 2; // Increase for even smoother curves

  for (int i = 0; i < iterations; ++i) {
    std::vector<glm::dvec2> new_points;
    new_points.push_back(smooth_points.front()); // Keep the start

    for (size_t j = 0; j < smooth_points.size() - 1; ++j) {
      glm::dvec2 p0 = smooth_points[j];
      glm::dvec2 p1 = smooth_points[j + 1];

      // Cut corners at 25% and 75%
      new_points.push_back(0.75 * p0 + 0.25 * p1);
      new_points.push_back(0.25 * p0 + 0.75 * p1);
    }
    new_points.push_back(smooth_points.back()); // Keep the end
    smooth_points = std::move(new_points);
  }

  // 2. Generate Render Geometry
  m_render_vertices.clear();
  double running_v = 0.0;
  double radius = m_thickness / 2.0;
  double miter_limit = radius * 4.0;

  for (size_t i = 0; i < smooth_points.size(); ++i) {
    glm::dvec2 curr = smooth_points[i];
    glm::dvec2 miter_normal;
    double length = radius;

    if (i == 0) {
      glm::dvec2 t = glm::normalize(smooth_points[1] - curr);
      miter_normal = glm::dvec2(-t.y, t.x);

      // Extension for Rounded Cap
      glm::dvec2 cap_origin = curr - (t * radius);
      m_render_vertices.push_back(
          {{static_cast<float>((cap_origin + miter_normal * radius).x),
            static_cast<float>((cap_origin + miter_normal * radius).y)},
           m_color,
           {0.0f, static_cast<float>(-radius)},
           static_cast<float>(m_thickness),
           0.0f});
      m_render_vertices.push_back(
          {{static_cast<float>((cap_origin - miter_normal * radius).x),
            static_cast<float>((cap_origin - miter_normal * radius).y)},
           m_color,
           {1.0f, static_cast<float>(-radius)},
           static_cast<float>(m_thickness),
           0.0f});
    } else if (i == smooth_points.size() - 1) {
      glm::dvec2 prev = smooth_points[i - 1];
      running_v += glm::distance(curr, prev);
      glm::dvec2 t = glm::normalize(curr - prev);
      miter_normal = glm::dvec2(-t.y, t.x);

      // Extension for Rounded Cap
      glm::dvec2 cap_origin = curr + (t * radius);
      m_render_vertices.push_back(
          {{static_cast<float>((cap_origin + miter_normal * radius).x),
            static_cast<float>((cap_origin + miter_normal * radius).y)},
           m_color,
           {0.0f, static_cast<float>(running_v + radius)},
           static_cast<float>(m_thickness),
           0.0f});
      m_render_vertices.push_back(
          {{static_cast<float>((cap_origin - miter_normal * radius).x),
            static_cast<float>((cap_origin - miter_normal * radius).y)},
           m_color,
           {1.0f, static_cast<float>(running_v + radius)},
           static_cast<float>(m_thickness),
           0.0f});
    } else {
      glm::dvec2 prev = smooth_points[i - 1];
      glm::dvec2 next = smooth_points[i + 1];
      running_v += glm::distance(curr, prev);

      glm::dvec2 t1 = glm::normalize(curr - prev);
      glm::dvec2 t2 = glm::normalize(next - curr);
      glm::dvec2 n1(-t1.y, t1.x);
      glm::dvec2 n2(-t2.y, t2.x);

      miter_normal = glm::normalize(n1 + n2);
      double dot = glm::dot(miter_normal, n1);
      length = radius / glm::max(0.1, dot);
      if (length > miter_limit)
        length = miter_limit;

      m_render_vertices.push_back(
          {{static_cast<float>((curr + miter_normal * length).x),
            static_cast<float>((curr + miter_normal * length).y)},
           m_color,
           {0.0f, static_cast<float>(running_v)},
           static_cast<float>(m_thickness),
           0.0f});
      m_render_vertices.push_back(
          {{static_cast<float>((curr - miter_normal * length).x),
            static_cast<float>((curr - miter_normal * length).y)},
           m_color,
           {1.0f, static_cast<float>(running_v)},
           static_cast<float>(m_thickness),
           0.0f});
    }
  }

  // 3. Finalize and Bake
  m_cummulative_distance = running_v;

  float min_x = std::numeric_limits<float>::max();
  float min_y = std::numeric_limits<float>::max();
  float max_x = std::numeric_limits<float>::lowest();
  float max_y = std::numeric_limits<float>::lowest();

  for (auto &v : m_render_vertices) {
    v.total_stroke_length = m_cummulative_distance;

    if (v.position.x < min_x)
      min_x = v.position.x;
    if (v.position.y < min_y)
      min_y = v.position.y;
    if (v.position.x > max_x)
      max_x = v.position.x;
    if (v.position.y > max_y)
      max_y = v.position.y;
  }

  if (m_render_vertices.empty()) {
    m_bounds = {{0.0f, 0.0f}, {0.0f, 0.0f}};
  } else {
    m_bounds = {{min_x, min_y}, {max_x, max_y}};
  }
}

void Stroke::set_color(glm::vec3 color) {
  m_color = color;
  std::cout << m_color.r << "," << m_color.g << "," << m_color.b << "\n";
}
void Stroke::set_thickness(double thickness) {
  m_thickness = thickness;

  for (PointVertex &point : m_render_vertices) {
    point.thickness = thickness;
  }
}

const std::vector<glm::dvec2> &Stroke::get_raw_points() const {
  return m_raw_points;
}

void Stroke::upload() {
  if (m_vbo == 0)
    glCreateBuffers(1, &m_vbo);

  size_t size = m_render_vertices.size() * sizeof(PointVertex);
  if (size == 0)
    return;

  glNamedBufferData(m_vbo, size, nullptr, GL_DYNAMIC_DRAW);
  glNamedBufferSubData(m_vbo, 0, size, m_render_vertices.data());
}

void Stroke::draw(GLuint &vao, const Shader &shader) const {
  glVertexArrayVertexBuffer(vao, 0, m_vbo, 0, sizeof(PointVertex));
  glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)m_render_vertices.size());
}

glm::vec3 Stroke::get_color() const { return m_color; }

double Stroke::get_thickness() const { return m_thickness; }

bool Stroke::is_empty() const { return m_render_vertices.empty(); }
