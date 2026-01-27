#include <math.h>

#include "geometry.h"

int create_circle(float *buffer, size_t buffer_size,
                  struct circle_config config) {

  if (config.definition == 0)
    return -1;
  if (config.definition * FLOATS_PER_TRIANGLE > buffer_size)
    return -1;

  const double angle_step = (2 * M_PI) / config.definition;

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

    size_t base_index = i * FLOATS_PER_TRIANGLE;

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

  return FLOATS_PER_TRIANGLE * config.definition;
}
