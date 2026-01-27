#define _USE_MATH_DEFINES // Required for M_PI on some compilers (MSVC)
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include "geometry.h"
#include "utils.h"

// Fallback if ASSETS_PATH is not defined
#ifndef ASSETS_PATH
#define ASSETS_PATH "./assets"
#endif

#define V_BUFFER_SIZE 503000
#define V_BUFFER_STACK_THRESHOLD 50000

//________________________________________________CALLBACKS__________________________________________________________//

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void process_input(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, 1);
}

//_________________________________________________SETUP_____________________________________________________________//

GLFWwindow *initialize_window(int width, int height, const char *title) {
  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  GLFWwindow *window = glfwCreateWindow(width, height, title, NULL, NULL);
  if (!window) {
    fprintf(stderr, "Failed to create GLFW window\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
    fprintf(stderr, "Failed to initialize GLAD\n");
    exit(EXIT_FAILURE);
  }
  return window;
}

GLuint create_shader_program(const char *vertex_path,
                             const char *fragment_path) {
  char *v_src = read_file(vertex_path);
  char *f_src = read_file(fragment_path);
  if (!v_src || !f_src)
    exit(EXIT_FAILURE);

  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, (const char *const *)&v_src, NULL);
  glCompileShader(vs);
  // (Error checking skipped for brevity, but recommended)

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, (const char *const *)&f_src, NULL);
  glCompileShader(fs);

  GLuint prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);

  glDeleteShader(vs);
  glDeleteShader(fs);
  free(v_src);
  free(f_src);
  return prog;
}

//_________________________________________________MAIN______________________________________________________________//

int main() {
  GLFWwindow *window = initialize_window(800, 600, "Procedural Circles");

  // Create Shaders
  GLuint shader_program =
      create_shader_program(ASSETS_PATH "/shaders/circle.vert.glsl",
                            ASSETS_PATH "/shaders/circle.frag.glsl");

#if V_BUFFER_SIZE > V_BUFFER_STACK_THRESHOLD
  float *v_buffer = (float *)malloc(V_BUFFER_SIZE * sizeof(float));
  if (!v_buffer) {
    fprintf(stderr, "Failed to allocate memory\n");
    return -1;
  }
#else
  float v_buffer[V_BUFFER_SIZE];
#endif

  size_t offset = 0;
  int r;

  // Main circle
  r = create_circle(v_buffer + offset, V_BUFFER_SIZE - offset,
                    (struct circle_config){.center_x = 0.0f,
                                           .center_y = 0.0f,
                                           .radius = 0.9f,
                                           .definition = 8,
                                           .color_seed = 0,
                                           .rotate_speed = 0.4f,
                                           .brightness = 1.0f});
  if (r > 0)
    offset += r;

  // Secondary circle
  r = create_circle(v_buffer + offset, V_BUFFER_SIZE - offset,
                    (struct circle_config){.center_x = 0.0f,
                                           .center_y = 0.0f,
                                           .radius = 0.75f,
                                           .definition = 5,
                                           .color_seed = 123,
                                           .rotate_speed = -1.0f,
                                           .brightness = 0.8f});
  if (r > 0)
    offset += r;

  // 100 Nested Levels
  const size_t TOTAL_LEVEL = 100;
  const float MIN_BRIGHTNESS = 0.6f;

  for (size_t level = 0; level < TOTAL_LEVEL; ++level) {
    size_t def = (level + 1) * 4;
    float speed = 1.0f + (level * 0.4f);
    float rad = 0.15f * powf(0.88f, level);
    float dist = (level + 1) * 0.15f;
    double angle_step = (2 * M_PI) / def;
    float brightness =
        MIN_BRIGHTNESS + ((float)level / TOTAL_LEVEL) * (1.0f - MIN_BRIGHTNESS);

    for (size_t i = 0; i < def; ++i) {
      double degree = i * angle_step;
      float c_x = 0.0f + dist * (float)cos(degree);
      float c_y = 0.0f + dist * (float)sin(degree);

      r = create_circle(
          v_buffer + offset, V_BUFFER_SIZE - offset,
          (struct circle_config){.center_x = c_x,
                                 .center_y = c_y,
                                 .radius = rad,
                                 .definition =
                                     10 * (TOTAL_LEVEL - level) / TOTAL_LEVEL,
                                 .color_seed = (int)degree,
                                 .rotate_speed = speed * ((level % 2) ? 1 : -1),
                                 .brightness = brightness});
      if (r > 0)
        offset += r;
    }
  }

  // 2. Upload to GPU
  GLuint vao, vbo;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, offset * sizeof(float), v_buffer,
               GL_STATIC_DRAW);

  // Attribute 0: Position (3 floats)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                        FLOATS_PER_VERTEX * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Attribute 1: Color (3 floats)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                        FLOATS_PER_VERTEX * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Attribute 2: Rotate Speed (1 float)
  glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE,
                        FLOATS_PER_VERTEX * sizeof(float),
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  // Attribute 3: Brightness (1 float)
  glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE,
                        FLOATS_PER_VERTEX * sizeof(float),
                        (void *)(7 * sizeof(float)));
  glEnableVertexAttribArray(3);

// Cleanup CPU memory
#if V_BUFFER_SIZE > V_BUFFER_STACK_THRESHOLD
  free(v_buffer);
#endif

  // 3. Render State
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);

  int loc_time = glGetUniformLocation(shader_program, "u_time");
  int loc_aspect = glGetUniformLocation(shader_program, "u_aspect");
  int vertex_count = (int)(offset / FLOATS_PER_VERTEX);

  while (!glfwWindowShouldClose(window)) {
    process_input(window);

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    float aspect = (float)w / (float)h;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);
    glUniform1f(loc_time, (float)glfwGetTime());
    glUniform1f(loc_aspect, aspect);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteProgram(shader_program);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
