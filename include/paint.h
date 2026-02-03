#pragma once

#include "glad/gl.h"

#include "GLFW/glfw3.h"
#include "glm/fwd.hpp"
#include "shader.h"
#include "stroke.h"
#include <vector>

#ifndef SHADER_PATH
#define SHADER_PATH ASSETS_PATH "/shaders"
#endif

class PaintApp {
public:
  const char *CIRCLE_VERTEX_SHADER_PATH = SHADER_PATH "/circle.vert.glsl";
  const char *CIRCLE_FRAGMENT_SHADER_PATH = SHADER_PATH "/circle.frag.glsl";

private:
  GLFWwindow *m_window;
  Shader m_circle_shader;
  GLuint m_vao, m_vbo;
  int m_width{0}, m_height{0};

  Stroke m_current_stroke;
  std::vector<Stroke> m_strokes;

  bool m_is_drawing{false};

  glm::mat4 m_projection;

public:
  PaintApp(GLFWwindow *window);
  ~PaintApp();

  void render();

  // GLFW adapter handler
  static void glfw_cursor_callback(GLFWwindow *window, double xpos,
                                   double ypos);
  static void glfw_mouse_button_callback(GLFWwindow *window, int button,
                                         int action, int mods);
  static void glfw_framebuffer_size_callback(GLFWwindow *window, int width,
                                             int height);

private:
  // internal Handlers
  void handle_viewport_size(int width, int height);
  void handle_mouse_click(int button, int action);
  void handle_mouse_move(double x, double y);

  // Drawing handlers
  void start_drawing();
  void on_drawing(double x, double y);
  void end_drawing();

  void setup_buffers();
};
