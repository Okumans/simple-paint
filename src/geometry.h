#ifndef GEOMETRY
#define GEOMETRY

#define FLOATS_PER_VERTEX 8
#define FLOATS_PER_TRIANGLE (FLOATS_PER_VERTEX * 3)

#include <stddef.h>

struct mesh_buffer {
  float *data;
  size_t capacity;
  size_t offset;
  int error;
};

struct mesh_buffer mesh_buffer_new_from_buffer(float *buffer, size_t capacity);

struct circle_config {
  float center_x;
  float center_y;
  float radius;
  size_t definition;
  int color_seed;
  float rotate_speed;
  float brightness;
};

enum circle_error {
  CIRCLE_ERR_NONE = 0,
  CIRCLE_ERR_INVALID_DEF = -1,
  CIRCLE_ERR_BUFFER_OVERFLOW = -2
};

void append_circle(struct mesh_buffer *buffer, struct circle_config config);

#endif // GEOMETRY
