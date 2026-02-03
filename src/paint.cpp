#include "paint.h"
#include "GLFW/glfw3.h"
#include "glad/gl.h"
#include "shader.h"
#include "stroke.h"
#include "ui_manager.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#include <vector>

PaintApp::PaintApp(GLFWwindow *window)
    : m_window(window), m_stroke_shader(Shader(STROKE_VERTEX_SHADER_PATH,
                                               STROKE_FRAGMENT_SHADER_PATH)),
      m_ui_shader(Shader(UI_VERTEX_SHADER_PATH, UI_FRAGMENT_SHADER_PATH)) {
  setup_buffers();

  glfwSetWindowUserPointer(m_window, (void *)this);

  glfwSetKeyCallback(m_window, PaintApp::glfw_key_callback);
  glfwSetCursorPosCallback(m_window, PaintApp::glfw_cursor_callback);
  glfwSetMouseButtonCallback(m_window, PaintApp::glfw_mouse_button_callback);
  glfwSetWindowSizeCallback(m_window, PaintApp::glfw_framebuffer_size_callback);
  glfwSetScrollCallback(m_window, PaintApp::glfw_scroll_callback);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Set up color picker
  {
    const float BOX_SIZE = 40.0f;
    const float PADDING = 2.0f;
    const float START_X = 10.0f;
    const float START_Y = 10.0f;

    std::vector<glm::vec3> palette = {
        {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.5f}, {0.5f, 0.0f, 0.0f},
        {0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 0.5f}, {0.0f, 0.0f, 0.0f},
        {1.0f, 0.5f, 0.0f}, {0.5f, 1.0f, 0.0f}, {0.5f, 0.0f, 1.0f},
        {0.0f, 0.5f, 1.0f}};

    for (int i = 0; i < palette.size(); ++i) {
      int row = i / 8;
      int col = i % 8;

      float x = START_X + col * (BOX_SIZE + PADDING);
      float y = START_Y + row * (BOX_SIZE + PADDING);

      glm::vec3 color = palette[i];
      std::string name = "Color_" + std::to_string(i);

      m_ui_manager.add_element(name, {x, y, BOX_SIZE, BOX_SIZE}, color,
                               [this, color]() { this->set_color(color); });
    }
  }
}

void PaintApp::setup_buffers() {
  glCreateVertexArrays(1, &m_stroke_vao);

  // Attribute 0: Position (3 floats)
  glEnableVertexArrayAttrib(m_stroke_vao, 0);
  glVertexArrayAttribFormat(m_stroke_vao, 0, 2, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, position));
  glVertexArrayAttribBinding(m_stroke_vao, 0, 0);

  // Attribute 1: Color (3 floats)
  glEnableVertexArrayAttrib(m_stroke_vao, 1);
  glVertexArrayAttribFormat(m_stroke_vao, 1, 3, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, color));
  glVertexArrayAttribBinding(m_stroke_vao, 1, 0);

  // Attribute 2: UV (2 floats)
  glEnableVertexArrayAttrib(m_stroke_vao, 2);
  glVertexArrayAttribFormat(m_stroke_vao, 2, 2, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, uv));
  glVertexArrayAttribBinding(m_stroke_vao, 2, 0);

  // Attribute 3: Total Stroke Length (1 floats)
  glEnableVertexArrayAttrib(m_stroke_vao, 3);
  glVertexArrayAttribFormat(m_stroke_vao, 3, 1, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, thickness));
  glVertexArrayAttribBinding(m_stroke_vao, 3, 0);

  // Attribute 3: Total Stroke Length (1 floats)
  glEnableVertexArrayAttrib(m_stroke_vao, 4);
  glVertexArrayAttribFormat(m_stroke_vao, 4, 1, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, total_stroke_length));
  glVertexArrayAttribBinding(m_stroke_vao, 4, 0);

  setup_brush_preview();
}

void PaintApp::render(double delta_time) {
  process_input();
  update_camera(delta_time);

  glBindVertexArray(m_stroke_vao);

  m_stroke_shader.use();
  m_stroke_shader.setMat4("u_projection", m_app_state.projection);

  for (auto &stroke : m_strokes) {
    stroke.draw(m_stroke_vao, m_stroke_shader);
  }

  if (!m_current_stroke.is_empty()) {
    m_current_stroke.draw(m_stroke_vao, m_stroke_shader);
  }

  if (!m_app_state.is_drawing) {
    glm::vec2 world_pos = screen_to_world(m_app_state, m_input_state.curr_pos.x,
                                          m_input_state.curr_pos.y);

    m_ui_shader.use();
    m_ui_shader.setMat4("u_projection", m_app_state.projection);

    // Transformation Matrix: Translate to mouse -> Scale to brush radius
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(world_pos, 0.0f));
    float radius = m_app_state.current_thickness * 0.5f;
    model = glm::scale(model, glm::vec3(radius, radius, 1.0f));

    m_ui_shader.setMat4("u_model", model);
    m_ui_shader.setVec3("u_color", m_app_state.current_color);
    m_ui_shader.setBool("u_hasTexture", false);
    m_ui_shader.setFloat("u_alpha", 0.4f);

    glBindVertexArray(m_preview_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, PREVIEW_SEGMENTS + 2);
  }

  m_ui_manager.render(m_ui_shader, m_app_state.window_width,
                      m_app_state.window_height);
}

void PaintApp::start_drawing() {
  m_app_state.is_drawing = true;
  m_strokes_revert.clear();
}

void PaintApp::on_drawing(double x, double y) {
  glm::vec2 world_pos = screen_to_world(m_app_state, x, y);

  m_current_stroke.add_point(world_pos.x, world_pos.y);
  m_current_stroke.upload();
}

void PaintApp::end_drawing() {
  m_app_state.is_drawing = false;
  if (!m_current_stroke.is_empty()) {
    m_current_stroke.update_geometry();
    m_current_stroke.upload();
    m_strokes.push_back(std::move(m_current_stroke));
    m_current_stroke =
        Stroke(m_app_state.current_color, m_app_state.current_thickness);
  }
}

// Paint app internal handlers
void PaintApp::update_camera(double deltaTime) {
  // 1. Smoothly interpolate Zoom
  // Formula: current = current + (target - current) * speed * dt
  m_app_state.zoom = glm::mix(m_app_state.zoom, m_app_state.target_zoom,
                              m_app_state.lerp_speed * deltaTime);

  // 2. Smoothly interpolate Position
  m_app_state.view_pos =
      glm::mix(m_app_state.view_pos, m_app_state.target_view_pos,
               m_app_state.lerp_speed * deltaTime);

  // 3. Update the projection matrix based on CURRENT (interpolated) values
  update_projection();
}

void PaintApp::handle_viewport_size(int width, int height) {
  m_app_state.window_width = width;
  m_app_state.window_height = height;

  glViewport(0, 0, width, height);

  update_projection();
}

void PaintApp::handle_scroll(double xoffset, double yoffset) {
  if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);

    glm::vec2 mouse_world_before = screen_to_world(m_app_state, x, y);

    // Set new zoom target
    if (yoffset > 0)
      m_app_state.target_zoom *= 0.9f;
    else
      m_app_state.target_zoom *= 1.1f;
    m_app_state.target_zoom =
        glm::clamp(m_app_state.target_zoom, 0.01f, 100.0f);

    // To keep the mouse "anchored" to the same world spot:
    // We calculate what the world position would be at the TARGET zoom
    // and adjust the target_view_pos to compensate.
    float nx = (2.0f * (float)x) / m_app_state.window_width - 1.0f;
    float ny = 1.0f - (2.0f * (float)y) / m_app_state.window_height;

    m_app_state.target_view_pos.x =
        mouse_world_before.x -
        (nx * m_app_state.get_aspect() * m_app_state.target_zoom);
    m_app_state.target_view_pos.y =
        mouse_world_before.y - (ny * m_app_state.target_zoom);
  } else {
    // Touchpad Panning
    float pan_speed = 0.05f * m_app_state.target_zoom;
    m_app_state.target_view_pos.x -= static_cast<float>(xoffset) * pan_speed;
    m_app_state.target_view_pos.y += static_cast<float>(yoffset) * pan_speed;
  }
}

void PaintApp::handle_key_event(int key, int action, int mods) {
  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_0) {
      m_app_state.target_view_pos = glm::vec2(0.0f, 0.0f);
      m_app_state.target_zoom = 1.0f;
    }

    bool ctrl_down = (mods & GLFW_MOD_CONTROL);

    // Undo: Ctrl + Z
    if (ctrl_down && key == GLFW_KEY_Z) {
      if (!m_strokes.empty()) {
        m_strokes_revert.push_back(std::move(m_strokes.back()));
        m_strokes.pop_back();
      }
    }

    // Redo: Ctrl + R
    if (ctrl_down && key == GLFW_KEY_R) {
      if (!m_strokes_revert.empty()) {
        m_strokes.push_back(std::move(m_strokes_revert.back()));
        m_strokes_revert.pop_back();
      }
    }

    if ((ctrl_down && key == GLFW_KEY_EQUAL)) {
      set_thickness(m_app_state.current_thickness * 1.2f);
    }

    if (ctrl_down && key == GLFW_KEY_MINUS) {
      set_thickness(m_app_state.current_thickness * 0.8f);
    }
  }
}

void PaintApp::handle_mouse_click(int button, int action) {
  if (m_ui_manager.handle_click(m_input_state.curr_pos.x,
                                m_input_state.curr_pos.y)) {
    m_input_state.is_pressed = true;
    return;
  }
  // IGNORE UI MANAGER FOR NOW

  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      m_input_state.is_pressed = true;
      start_drawing();
    } else if (action == GLFW_RELEASE) {
      m_input_state.is_pressed = false;
      end_drawing();
    }
  }
}

void PaintApp::process_input() {
  double x, y;

  glfwGetCursorPos(m_window, &x, &y);

  m_input_state.update_pos({x, y});

  bool left_down =
      glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  bool right_down =
      glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
  bool space_down = glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS;

  bool is_panning_chord = left_down && right_down;
  bool is_space_panning = space_down && left_down;

  if (is_panning_chord || is_space_panning) {
    // Safety: Cancel stroke if we start panning while drawing
    if (m_app_state.is_drawing) {
      m_current_stroke =
          Stroke(m_app_state.current_color, m_app_state.current_thickness);
      m_app_state.is_drawing = false;
    }

    glm::vec2 delta = m_input_state.curr_pos - m_input_state.prev_pos;
    float aspect = m_app_state.get_aspect();

    m_app_state.target_view_pos.x -= (delta.x / m_app_state.window_width) *
                                     2.0f * aspect * m_app_state.target_zoom;
    m_app_state.target_view_pos.y +=
        (delta.y / m_app_state.window_height) * 2.0f * m_app_state.target_zoom;
  }
}

void PaintApp::handle_mouse_move(double x, double y) {
  // We check panning conditions to ensure we don't draw while panning chords
  // are active
  bool left_down =
      glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  bool right_down =
      glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
  bool space_down = glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS;
  bool is_panning = (left_down && right_down) || (space_down && left_down);

  if (m_app_state.is_drawing && !is_panning) {
    on_drawing(x, y);
  }
}

// Helper methods

void PaintApp::set_color(glm::vec3 color) {
  m_app_state.current_color = color;
  m_current_stroke.set_color(color);
}

void PaintApp::set_thickness(float thickness) {
  m_app_state.current_thickness = thickness;
  m_current_stroke.set_thickness(thickness);
}

glm::vec2 PaintApp::screen_to_world(const AppState &state, double xpos,
                                    double ypos) {
  float nx = (2.0f * (float)xpos) / state.window_width - 1.0f;
  float ny = 1.0f - (2.0f * (float)ypos) / state.window_height;

  // 2. World Space Transform
  // x = (NDC_x * Aspect * Zoom) + Pan_x
  // y = (NDC_y * Zoom) + Pan_y
  glm::vec2 world;
  world.x = (nx * state.get_aspect() * state.zoom) + state.view_pos.x;
  world.y = (ny * state.zoom) + state.view_pos.y;

  return world;
}

void PaintApp::update_projection() {
  float a = m_app_state.get_aspect() * m_app_state.zoom;
  float z = m_app_state.zoom;

  // The bounds of the camera in World Space
  float left = m_app_state.view_pos.x - a;
  float right = m_app_state.view_pos.x + a;
  float bottom = m_app_state.view_pos.y - z;
  float top = m_app_state.view_pos.y + z;

  m_app_state.projection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
}

void PaintApp::setup_brush_preview() {
  // Create a simple circle with 32 segments
  std::vector<float> vertices;
  vertices.push_back(0.0f); // Center X
  vertices.push_back(0.0f); // Center Y

  int segments = 32;
  for (int i = 0; i <= segments; ++i) {
    float angle = i * 2.0f * glm::pi<float>() / segments;
    vertices.push_back(cos(angle));
    vertices.push_back(sin(angle));
  }

  glCreateVertexArrays(1, &m_preview_vao);
  glCreateBuffers(1, &m_preview_vbo);

  glNamedBufferStorage(m_preview_vbo, vertices.size() * sizeof(float),
                       vertices.data(), 0);

  glVertexArrayVertexBuffer(m_preview_vao, 0, m_preview_vbo, 0,
                            2 * sizeof(float));
  glEnableVertexArrayAttrib(m_preview_vao, 0);
  glVertexArrayAttribFormat(m_preview_vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(m_preview_vao, 0, 0);
}

// GLFW adapter handlers

void PaintApp::glfw_framebuffer_size_callback(GLFWwindow *window, int width,
                                              int height) {
  auto *app = static_cast<PaintApp *>(glfwGetWindowUserPointer(window));
  if (app) {
    app->handle_viewport_size(width, height);
  }
}

void PaintApp::glfw_scroll_callback(GLFWwindow *window, double xoffset,
                                    double yoffset) {
  auto *app = static_cast<PaintApp *>(glfwGetWindowUserPointer(window));
  if (app) {
    app->handle_scroll(xoffset, yoffset);
  }
}

void PaintApp::glfw_cursor_callback(GLFWwindow *window, double xpos,
                                    double ypos) {
  auto *app = static_cast<PaintApp *>(glfwGetWindowUserPointer(window));
  if (app)
    app->handle_mouse_move(xpos, ypos);
}

void PaintApp::glfw_key_callback(GLFWwindow *window, int key, int scancode,
                                 int action, int mods) {
  auto *app = static_cast<PaintApp *>(glfwGetWindowUserPointer(window));
  if (app)
    app->handle_key_event(key, action, mods);
}

void PaintApp::glfw_mouse_button_callback(GLFWwindow *window, int button,
                                          int action, int mods) {
  auto *app = static_cast<PaintApp *>(glfwGetWindowUserPointer(window));
  if (app)
    app->handle_mouse_click(button, action);
}

PaintApp::~PaintApp() {
  glDeleteVertexArrays(1, &m_stroke_vao);
  glDeleteProgram(m_stroke_shader.ID);
}
