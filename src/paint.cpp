#include "paint.h"
#include "GLFW/glfw3.h"
#include "glad/gl.h"
#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#include <vector>

PaintApp::PaintApp(GLFWwindow *window)
    : m_window(window), m_stroke_shader(Shader(STROKE_VERTEX_SHADER_PATH,
                                               STROKE_FRAGMENT_SHADER_PATH)),
      m_projection(1.0f) {

  setup_buffers();

  glfwSetWindowUserPointer(m_window, (void *)this);

  glfwSetCursorPosCallback(m_window, PaintApp::glfw_cursor_callback);
  glfwSetMouseButtonCallback(m_window, PaintApp::glfw_mouse_button_callback);
  glfwSetWindowSizeCallback(m_window, PaintApp::glfw_framebuffer_size_callback);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void PaintApp::setup_buffers() {
  glCreateVertexArrays(1, &m_vao);

  // Attribute 0: Position (3 floats)
  glEnableVertexArrayAttrib(m_vao, 0);
  glVertexArrayAttribFormat(m_vao, 0, 2, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, position));
  glVertexArrayAttribBinding(m_vao, 0, 0);

  // Attribute 1: Color (3 floats)
  glEnableVertexArrayAttrib(m_vao, 1);
  glVertexArrayAttribFormat(m_vao, 1, 3, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, color));
  glVertexArrayAttribBinding(m_vao, 1, 0);

  // Attribute 2: UV (2 floats)
  glEnableVertexArrayAttrib(m_vao, 2);
  glVertexArrayAttribFormat(m_vao, 2, 2, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, uv));
  glVertexArrayAttribBinding(m_vao, 2, 0);

  // Attribute 3: Total Stroke Length (1 floats)
  glEnableVertexArrayAttrib(m_vao, 3);
  glVertexArrayAttribFormat(m_vao, 3, 1, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, total_stroke_length));
  glVertexArrayAttribBinding(m_vao, 3, 0);
}

void PaintApp::render() {
  m_stroke_shader.use();
  m_stroke_shader.setMat4("u_projection", m_projection);

  glBindVertexArray(m_vao);

  for (auto &stroke : m_strokes) {
    stroke.draw(m_vao, m_stroke_shader);
  }

  if (!m_current_stroke.is_empty()) {
    m_current_stroke.draw(m_vao, m_stroke_shader);
  }
}

void PaintApp::start_drawing() { m_is_drawing = true; }

void PaintApp::on_drawing(double x, double y) {
  float aspectRatio =
      static_cast<float>(m_width) / static_cast<float>(m_height);

  float norm_x = (2.0f * static_cast<float>(x)) / m_width - 1.0f;
  float norm_y = 1.0f - (2.0f * static_cast<float>(y)) / m_height;

  float final_x = norm_x * aspectRatio;
  float final_y = norm_y;

  m_current_stroke.add_point(final_x, final_y);
  m_current_stroke.upload();
}

void PaintApp::end_drawing() {
  m_is_drawing = false;
  if (!m_current_stroke.is_empty()) {
    m_current_stroke.update_geometry();
    m_current_stroke.upload();
    m_strokes.push_back(std::move(m_current_stroke));
    m_current_stroke = Stroke();
  }
}

// Paint app internal handlers

void PaintApp::handle_viewport_size(int width, int height) {
  m_width = width;
  m_height = height;

  glViewport(0, 0, width, height);

  float aspectRatio = (float)width / (float)height;
  m_projection = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f);
}

void PaintApp::handle_mouse_move(double x, double y) {
  if (m_is_drawing) {
    input_state.update_pos({x, y});
    on_drawing(x, y);
  }
}

void PaintApp::handle_mouse_click(int button, int action) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      input_state.is_pressed = true;
      start_drawing();
    } else if (action == GLFW_RELEASE) {
      input_state.is_pressed = true;
      end_drawing();
    }
  }
}

// GLFW adapter handlers

void PaintApp::glfw_framebuffer_size_callback(GLFWwindow *window, int width,
                                              int height) {
  auto *app = static_cast<PaintApp *>(glfwGetWindowUserPointer(window));
  if (app) {
    app->handle_viewport_size(width, height);
    std::cout << "frame size changed" << width << ", " << height << '\n';
  }
}

void PaintApp::glfw_cursor_callback(GLFWwindow *window, double xpos,
                                    double ypos) {
  auto *app = static_cast<PaintApp *>(glfwGetWindowUserPointer(window));
  if (app)
    app->handle_mouse_move(xpos, ypos);
}

void PaintApp::glfw_mouse_button_callback(GLFWwindow *window, int button,
                                          int action, int mods) {
  auto *app = static_cast<PaintApp *>(glfwGetWindowUserPointer(window));
  if (app)
    app->handle_mouse_click(button, action);
}

PaintApp::~PaintApp() {
  glDeleteVertexArrays(1, &m_vao);
  glDeleteProgram(m_stroke_shader.ID);
}
