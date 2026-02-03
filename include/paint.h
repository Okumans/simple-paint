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

struct InputState {
  glm::vec2 curr_pos;
  glm::vec2 prev_pos;

  bool is_pressed = false;

  void update_pos(glm::vec2 new_pos) {
    prev_pos = curr_pos;
    curr_pos = new_pos;
  }

  glm::vec2 get_pos(double delta) {
    // TODO: implement lerp if needed
    return curr_pos;
  }
};

class PaintApp {
public:
  const char *STROKE_VERTEX_SHADER_PATH = SHADER_PATH "/stroke.vert.glsl";
  const char *STROKE_FRAGMENT_SHADER_PATH = SHADER_PATH "/stroke.frag.glsl";

private:
  GLFWwindow *m_window;
  Shader m_stroke_shader;
  GLuint m_vao, m_vbo;
  int m_width{0}, m_height{0};

  Stroke m_current_stroke;
  std::vector<Stroke> m_strokes;

  bool m_is_drawing{false};
  glm::mat4 m_projection;

  InputState input_state;

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
