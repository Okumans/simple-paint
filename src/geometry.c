#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "geometry.h"

struct mesh_buffer mesh_buffer_new_from_buffer(float *buffer, size_t capacity) {
  return (struct mesh_buffer){
      .data = buffer, .capacity = capacity, .offset = 0, .error = 0};
}

void append_circle(struct mesh_buffer *mb, struct circle_config config) {
  if (mb->error != CIRCLE_ERR_NONE)
    return;

  if (config.definition == 0) {
    mb->error = CIRCLE_ERR_INVALID_DEF;
    return;
  }

  size_t need_floats = config.definition * FLOATS_PER_TRIANGLE;

  if (mb->offset + need_floats > mb->capacity) {
    mb->error = CIRCLE_ERR_BUFFER_OVERFLOW;
    return;
  }

  float *buffer = mb->data + mb->offset;
  const double angle_step = (2 * M_PI) / config.definition;

  size_t base_index = 0;

  for (size_t i = 0; i < config.definition; ++i) {
    const double degree = i * angle_step;
    const double next_degree = (i + 1) * angle_step;

    float x_1 = config.center_x + config.radius * cos(degree);
    float y_1 = config.center_y + config.radius * sin(degree);
    float x_2 = config.center_x + config.radius * cos(next_degree);
    float y_2 = config.center_y + config.radius * sin(next_degree);

    float r_1 = sin(degree + config.color_seed) * 0.5f + 0.5f;
    float g_1 =
        sin(degree + +config.color_seed + 2.0f * M_PI / 3.0f) * 0.5f + 0.5f;
    float b_1 =
        sin(degree + +config.color_seed + 4.0f * M_PI / 3.0f) * 0.5f + 0.5f;

    float r_2 = sin(next_degree + config.color_seed) * 0.5f + 0.5f;
    float g_2 =
        sin(next_degree + config.color_seed + 2.0f * M_PI / 3.0f) * 0.5f + 0.5f;
    float b_2 =
        sin(next_degree + config.color_seed + 4.0f * M_PI / 3.0f) * 0.5f + 0.5f;

    // 1. First point
    buffer[base_index++] = x_1;
    buffer[base_index++] = y_1;
    buffer[base_index++] = 0.0f;
    // Color
    buffer[base_index++] = r_1;
    buffer[base_index++] = g_1;
    buffer[base_index++] = b_1;
    // Attributes
    buffer[base_index++] = config.rotate_speed;
    buffer[base_index++] = config.brightness;

    // 2. Center point
    buffer[base_index++] = config.center_x;
    buffer[base_index++] = config.center_y;
    buffer[base_index++] = 0.0f;
    // Color (White)
    buffer[base_index++] = 0.8f;
    buffer[base_index++] = 0.8f;
    buffer[base_index++] = 0.8f;
    // Attributes
    buffer[base_index++] = config.rotate_speed;
    buffer[base_index++] = config.brightness;

    // 3. Second point
    buffer[base_index++] = x_2;
    buffer[base_index++] = y_2;
    buffer[base_index++] = 0.0f;
    // Color
    buffer[base_index++] = r_2;
    buffer[base_index++] = g_2;
    buffer[base_index++] = b_2;
    // Attributes
    buffer[base_index++] = config.rotate_speed;
    buffer[base_index++] = config.brightness;
  }

  mb->offset += need_floats;
}
