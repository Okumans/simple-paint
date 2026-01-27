#ifndef GEOMETRY
#define GEOMETRY

#define FLOATS_PER_VERTEX 8
#define FLOATS_PER_TRIANGLE (FLOATS_PER_VERTEX * 3)

#include <stddef.h>

struct circle_config {
  float center_x;
  float center_y;
  float radius;
  size_t definition;
  int color_seed;
  float rotate_speed;
  float brightness;
};

int create_circle(float *buffer, size_t buffer_size, struct circle_config);

#endif // GEOMETRY
