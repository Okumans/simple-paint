#include "paint.h"
#include "GLFW/glfw3.h"
#include "geometry.h"
#include "glad/gl.h"
#include "shader.h"
#include "stroke.h"
#include "ui_manager.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

GLuint load_texture(const char *path) {
  GLuint textureID;
  glGenTextures(1, &textureID);

  int width, height, nrComponents;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
  if (data) {
    GLenum format;
    if (nrComponents == 1)
      format = GL_RED;
    else if (nrComponents == 3)
      format = GL_RGB;
    else if (nrComponents == 4)
      format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  } else {
    std::cout << "Texture failed to load at path: " << path << std::endl;
    stbi_image_free(data);
  }

  return textureID;
}

PaintApp::PaintApp(GLFWwindow *window)
    : m_window(window), m_stroke_shader(Shader(STROKE_VERTEX_SHADER_PATH,
                                               STROKE_FRAGMENT_SHADER_PATH)),
      m_ui_shader(Shader(UI_VERTEX_SHADER_PATH, UI_FRAGMENT_SHADER_PATH)),
      m_grid_shader(
          Shader(GRID_VERTEX_SHADER_PATH, GRID_FRAGMENT_SHADER_PATH)) {
  setup_buffers();

  glfwSetWindowUserPointer(m_window, (void *)this);

  glfwSetKeyCallback(m_window, PaintApp::glfw_key_callback);
  glfwSetCursorPosCallback(m_window, PaintApp::glfw_cursor_callback);
  glfwSetMouseButtonCallback(m_window, PaintApp::glfw_mouse_button_callback);
  glfwSetWindowSizeCallback(m_window, PaintApp::glfw_framebuffer_size_callback);
  glfwSetScrollCallback(m_window, PaintApp::glfw_scroll_callback);

  glEnable(GL_BLEND);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                      GL_ONE_MINUS_SRC_ALPHA);

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

      m_ui_manager.add_element(
          name, {x, y, BOX_SIZE, BOX_SIZE}, color, [this, color](auto _) {
            this->set_color(color);

            if (m_app_state.is_eraser) {
              this->m_app_state.is_eraser = false;
              this->m_ui_manager.get_element("current_tool")->textureID =
                  m_pen_tex;
            }
          });
    }
  }

  // Set up eraser
  m_eraser_tex = load_texture(ICONS_PATH "/eraser.png");
  m_pen_tex = load_texture(ICONS_PATH "/pen.png");

  m_ui_manager.add_element("current_tool", {374.0f, 10.0f, 40.0f, 40.0f},
                           m_pen_tex, [this](UIElement *self) {
                             this->m_app_state.is_eraser =
                                 !this->m_app_state.is_eraser;

                             if (this->m_app_state.is_eraser) {
                               self->textureID = m_eraser_tex;
                             } else {
                               self->textureID = m_pen_tex;
                             }
                           });
}

void setup_brush_preview(GLuint &preview_vao, GLuint &preview_vbo);
void setup_stroke(GLuint &preview_vao);

void PaintApp::setup_buffers() {
  setup_stroke(m_stroke_vao);
  setup_brush_preview(m_preview_vao, m_preview_vbo);
}

void PaintApp::render(double delta_time) {
  process_input();
  update_camera(delta_time);

  // --- STROKE RENDERING ---
  m_stroke_shader.use();
  glBindVertexArray(m_stroke_vao);
  m_stroke_shader.setMat4("u_projection", m_app_state.projection);

  double aspect_zoom =
      static_cast<double>(m_app_state.get_aspect()) * m_app_state.zoom;
  double zoom = static_cast<double>(m_app_state.zoom);
  AABB camera_bounds;
  camera_bounds.min = {m_app_state.view_pos.x - aspect_zoom,
                       m_app_state.view_pos.y - zoom};
  camera_bounds.max = {m_app_state.view_pos.x + aspect_zoom,
                       m_app_state.view_pos.y + zoom};

  for (auto &stroke : m_strokes) {
    if (!stroke.get_bounds().intersects(camera_bounds))
      continue;

    if (stroke.is_eraser()) {
      glBlendFuncSeparate(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO,
                          GL_ONE_MINUS_SRC_ALPHA);
    } else {
      glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                          GL_ONE_MINUS_SRC_ALPHA);
    }

    if (stroke.get_raw_points().size() == 1) {
      draw_dot(m_preview_vao, stroke.get_raw_points().front(),
               stroke.get_thickness() / 2.0f, stroke.get_color(), 1.0f);
    } else {
      m_stroke_shader.use();
      m_stroke_shader.setMat4("u_projection", m_app_state.projection);
      glBindVertexArray(m_stroke_vao);
      stroke.draw(m_stroke_vao, m_stroke_shader);
    }
  }

  // --- CURRENT STROKE + START CAP ---
  if (!m_current_stroke.is_empty()) {
    if (m_current_stroke.is_eraser()) {
      glBlendFuncSeparate(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO,
                          GL_ONE_MINUS_SRC_ALPHA);
    } else {
      glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                          GL_ONE_MINUS_SRC_ALPHA);
    }

    // Draw the "Live" Start Cap
    draw_dot(m_preview_vao, m_current_stroke.get_raw_points().front(),
             m_app_state.current_thickness / 2.0f, m_app_state.current_color,
             1.0f);

    // Draw the actual line
    m_stroke_shader.use(); // Ensure stroke shader is active for the ribbon
    m_stroke_shader.setMat4("u_projection", m_app_state.projection);
    glBindVertexArray(m_stroke_vao);
    m_current_stroke.draw(m_stroke_vao, m_stroke_shader);
  }

  // --- MOUSE PREVIEW ---
  // Reset to standard Alpha blending for the UI/Cursor
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glm::dvec2 world_pos = screen_to_world(m_app_state, m_input_state.curr_pos.x,
                                         m_input_state.curr_pos.y);

  if (m_app_state.is_eraser) {
    // White Ring for Eraser
    draw_dot(m_preview_vao, world_pos, m_app_state.current_thickness / 2.0f,
             {1.0f, 1.0f, 1.0f}, 0.6f, GL_LINE_LOOP);
  } else {
    // Solid colored dot for Pen
    draw_dot(m_preview_vao, world_pos, m_app_state.current_thickness / 2.0f,
             m_app_state.current_color, 0.4f, GL_TRIANGLE_FAN);
  }

  // --- GRID ---
  glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
  glm::mat4 gridModel =
      glm::translate(glm::mat4(1.0f), glm::vec3(m_app_state.view_pos, -0.9f));
  gridModel = glm::scale(
      gridModel, glm::vec3(m_app_state.get_aspect() * m_app_state.zoom * 2.0f,
                           m_app_state.zoom * 2.0f, 1.0f));
  gridModel = glm::translate(gridModel, glm::vec3(-0.5f, -0.5f, 0.0f));

  m_grid_shader.use();
  m_grid_shader.setMat4("u_projection", m_app_state.projection);
  m_grid_shader.setMat4("u_model", gridModel);
  m_grid_shader.setFloat("u_zoom", m_app_state.zoom);
  draw_quad();

  // --- UI ---
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  m_ui_manager.render(m_ui_shader, m_app_state.window_width,
                      m_app_state.window_height);
}

void PaintApp::start_drawing() {
  m_app_state.is_drawing = true;
  m_strokes_revert.clear();

  m_current_stroke =
      Stroke(m_app_state.current_color, m_app_state.current_thickness,
             m_app_state.is_eraser);

  glm::dvec2 world_pos = screen_to_world(m_app_state, m_input_state.curr_pos.x,
                                         m_input_state.curr_pos.y);

  m_current_stroke.add_point(world_pos.x, world_pos.y);
  m_current_stroke.upload();
}

void PaintApp::on_drawing(double x, double y) {
  glm::dvec2 world_pos = screen_to_world(m_app_state, x, y);

  m_current_stroke.add_point(world_pos.x, world_pos.y);
  m_current_stroke.upload();
}

void PaintApp::end_drawing() {
  m_app_state.is_drawing = false;
  if (!m_current_stroke.is_empty()) {
    if (m_current_stroke.get_raw_points().size() > 1) {
      m_current_stroke.update_geometry();
      m_current_stroke.upload();
    }

    m_strokes.push_back(std::move(m_current_stroke));
  }
  m_current_stroke =
      Stroke(m_app_state.current_color, m_app_state.current_thickness,
             m_app_state.is_eraser);
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

    glm::dvec2 mouse_world_before = screen_to_world(m_app_state, x, y);

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
        mouse_world_before.x - (static_cast<double>(nx) *
                                static_cast<double>(m_app_state.get_aspect()) *
                                static_cast<double>(m_app_state.target_zoom));
    m_app_state.target_view_pos.y =
        mouse_world_before.y - (static_cast<double>(ny) *
                                static_cast<double>(m_app_state.target_zoom));
  } else {
    // Touchpad Panning
    float pan_speed = 0.05f * m_app_state.target_zoom;
    m_app_state.target_view_pos.x -=
        static_cast<double>(xoffset) * static_cast<double>(pan_speed);
    m_app_state.target_view_pos.y +=
        static_cast<double>(yoffset) * static_cast<double>(pan_speed);
  }
}

void PaintApp::handle_key_event(int key, int action, int mods) {
  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_0) {
      m_app_state.target_view_pos = glm::vec2(0.0f, 0.0f);
      m_app_state.target_zoom = 1.0f;
    }

    bool ctrl_down = (mods & GLFW_MOD_CONTROL);

    // Undo: Ctrl + Z or U
    if ((ctrl_down && key == GLFW_KEY_Z) || key == GLFW_KEY_U) {
      if (!m_strokes.empty()) {
        m_strokes_revert.push_back(std::move(m_strokes.back()));
        m_strokes.pop_back();
      }
    }

    // Redo: Ctrl + Y or Ctrl + R
    if (ctrl_down && (key == GLFW_KEY_R || key == GLFW_KEY_Y)) {
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

    if (key == GLFW_KEY_E) {
      m_app_state.is_eraser = !m_app_state.is_eraser;
      UIElement *tool_el = m_ui_manager.get_element("current_tool");
      if (tool_el) {
        tool_el->textureID = m_app_state.is_eraser ? m_eraser_tex : m_pen_tex;
      }
    }
  }
}

void PaintApp::handle_mouse_click(int button, int action) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      m_input_state.is_pressed = true;

      if (m_ui_manager.handle_click(m_input_state.curr_pos.x,
                                    m_input_state.curr_pos.y)) {
        return;
      }

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

    glm::dvec2 delta = m_input_state.curr_pos - m_input_state.prev_pos;
    float aspect = m_app_state.get_aspect();

    m_app_state.target_view_pos.x -=
        (delta.x / static_cast<double>(m_app_state.window_width)) * 2.0 *
        static_cast<double>(aspect) *
        static_cast<double>(m_app_state.target_zoom);
    m_app_state.target_view_pos.y +=
        (delta.y / static_cast<double>(m_app_state.window_height)) * 2.0 *
        static_cast<double>(m_app_state.target_zoom);
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

void PaintApp::draw_dot(GLuint &vao, const glm::vec2 &world_pos, float radius,
                        const glm::vec3 &color, float alpha,
                        int draw_mode) const {
  // 1. Prepare Transformation
  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(world_pos, 0.0f));
  model = glm::scale(model, glm::vec3(radius, radius, 1.0f));

  // 2. Setup Shader State
  m_ui_shader.use();
  m_ui_shader.setMat4("u_projection", m_app_state.projection);
  m_ui_shader.setMat4("u_model", model);
  m_ui_shader.setVec3("u_color", color);
  m_ui_shader.setFloat("u_alpha", alpha);
  m_ui_shader.setBool("u_hasTexture", false);

  // 3. Draw
  glBindVertexArray(vao);
  if (draw_mode == GL_LINE_LOOP) {
    // Skip index 0 (center) to avoid the "spoke" line
    glDrawArrays(GL_LINE_LOOP, 1, 32);
  } else {
    glDrawArrays(GL_TRIANGLE_FAN, 0, 32 + 2);
  }
}

glm::dvec2 PaintApp::screen_to_world(const AppState &state, double xpos,
                                     double ypos) {
  float nx = (2.0f * (float)xpos) / state.window_width - 1.0f;
  float ny = 1.0f - (2.0f * (float)ypos) / state.window_height;

  // Direct inverse transform
  glm::mat4 invProj = glm::inverse(state.projection);
  glm::vec4 worldPos = invProj * glm::vec4(nx, ny, 0.0f, 1.0f);

  return glm::dvec2(worldPos);
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

void setup_stroke(GLuint &stroke_vao) {
  glCreateVertexArrays(1, &stroke_vao);

  // Attribute 0: Position (3 floats)
  glEnableVertexArrayAttrib(stroke_vao, 0);
  glVertexArrayAttribFormat(stroke_vao, 0, 2, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, position));
  glVertexArrayAttribBinding(stroke_vao, 0, 0);

  // Attribute 1: Color (3 floats)
  glEnableVertexArrayAttrib(stroke_vao, 1);
  glVertexArrayAttribFormat(stroke_vao, 1, 3, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, color));
  glVertexArrayAttribBinding(stroke_vao, 1, 0);

  // Attribute 2: UV (2 floats)
  glEnableVertexArrayAttrib(stroke_vao, 2);
  glVertexArrayAttribFormat(stroke_vao, 2, 2, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, uv));
  glVertexArrayAttribBinding(stroke_vao, 2, 0);

  // Attribute 3: Total Stroke Length (1 floats)
  glEnableVertexArrayAttrib(stroke_vao, 3);
  glVertexArrayAttribFormat(stroke_vao, 3, 1, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, thickness));
  glVertexArrayAttribBinding(stroke_vao, 3, 0);

  // Attribute 3: Total Stroke Length (1 floats)
  glEnableVertexArrayAttrib(stroke_vao, 4);
  glVertexArrayAttribFormat(stroke_vao, 4, 1, GL_FLOAT, GL_FALSE,
                            offsetof(PointVertex, total_stroke_length));
  glVertexArrayAttribBinding(stroke_vao, 4, 0);
}

void setup_brush_preview(GLuint &preview_vao, GLuint &preview_vbo) {
  const int segments = 32;
  std::vector<float> vertices;

  // Center point for GL_TRIANGLE_FAN
  vertices.push_back(0.0f);
  vertices.push_back(0.0f);

  for (int i = 0; i <= segments; ++i) {
    float angle = i * 2.0f * glm::pi<float>() / segments;
    vertices.push_back(cos(angle));
    vertices.push_back(sin(angle));
  }

  glCreateVertexArrays(1, &preview_vao);
  glCreateBuffers(1, &preview_vbo);

  glNamedBufferStorage(preview_vbo, vertices.size() * sizeof(float),
                       vertices.data(), 0);

  glVertexArrayVertexBuffer(preview_vao, 0, preview_vbo, 0, 2 * sizeof(float));
  glEnableVertexArrayAttrib(preview_vao, 0);
  glVertexArrayAttribFormat(preview_vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
  glVertexArrayAttribBinding(preview_vao, 0, 0);
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

  glDeleteVertexArrays(1, &m_preview_vao);
  glDeleteBuffers(1, &m_preview_vbo);

  glDeleteVertexArrays(1, &m_grid_vao);
  glDeleteBuffers(1, &m_grid_vbo);

  glDeleteTextures(1, &m_eraser_tex);
  glDeleteTextures(1, &m_pen_tex);

  glDeleteProgram(m_stroke_shader.ID);
  glDeleteProgram(m_ui_shader.ID);
  glDeleteProgram(m_grid_shader.ID);
}
