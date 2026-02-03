#pragma once

#include "glad/gl.h"

#include "GLFW/glfw3.h"
#include "glm/ext/vector_float2.hpp"
#include "glm/fwd.hpp"
#include "shader.h"
#include "stroke.h"
#include <stack>
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
};

struct AppState {
  // --- Canvas State ---
  glm::vec2 target_view_pos = {0.0f, 0.0f};
  glm::vec2 view_pos = {0.0f, 0.0f};

  float target_zoom = 1.0f;
  float zoom = 1.0f;

  float lerp_speed = 10.0f;

  // --- Tool State ---
  glm::vec3 current_color = {1.0f, 1.0f, 1.0f};
  float current_thickness = 0.05f;

  // --- Interaction State ---
  bool is_drawing = false;
  bool is_panning = false;

  // --- Viewport ---
  int window_width = 800;
  int window_height = 600;

  float get_aspect() const {
    return static_cast<float>(window_width) / static_cast<float>(window_height);
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

  Stroke m_current_stroke;
  std::vector<Stroke> m_strokes;
  std::vector<Stroke> m_strokes_revert; // for <C-R>

  glm::mat4 m_projection;

  InputState m_input_state;
  AppState m_app_state;

public:
  PaintApp(GLFWwindow *window);
  ~PaintApp();

  void render(double delta_time);

  // GLFW adapter handler
  static void glfw_cursor_callback(GLFWwindow *window, double xpos,
                                   double ypos);
  static void glfw_mouse_button_callback(GLFWwindow *window, int button,
                                         int action, int mods);
  static void glfw_key_callback(GLFWwindow *window, int key, int scancode,
                                int action, int mods);
  static void glfw_framebuffer_size_callback(GLFWwindow *window, int width,
                                             int height);
  static void glfw_scroll_callback(GLFWwindow *window, double xoffset,
                                   double yoffset);

private:
  // internal Handlers
  void process_input();
  void handle_key_event(int key, int action, int mods);
  void handle_viewport_size(int width, int height);
  void handle_mouse_click(int button, int action);
  void handle_mouse_move(double x, double y);
  void handle_scroll(double xoffset, double yoffset);

  // Drawing handlers
  void start_drawing();
  void on_drawing(double x, double y);
  void end_drawing();

  // Helper method
  void update_camera(double delta_time);
  void update_projection();
  static glm::vec2 screen_to_world(const AppState &state, double x, double yh);

  void setup_buffers();
};
