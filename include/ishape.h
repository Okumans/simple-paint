#pragma once

#include "shader.h"
#include <glad/gl.h>

class IShape {
public:
  virtual ~IShape() = default;

  // Logic: Math formulas, smoothing, bounding boxes
  virtual void update_geometry() = 0;

  // GPU Management
  virtual void upload() = 0;

  // Rendering: Needs to know which VAO/Shader to use
  virtual void draw(GLuint vao, const Shader &shader) const = 0;

  // Selection/Modification logic
  // virtual bool contains_point(float x, float y) const = 0;
};
