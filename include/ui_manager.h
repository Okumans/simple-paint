#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "shader.h"

struct UIHitbox {
  float x, y, w, h;

  bool operator<(const UIHitbox &other) const {
    if (x != other.x)
      return x < other.x;
    return y < other.y;
  }
  bool contains(double px, double py) const {
    return px >= x && px <= x + w && py >= y && py <= y + h;
  }
};

class UIElement {
public:
  std::string name;
  UIHitbox hitbox;
  std::function<void()> onClick;

  GLuint textureID = 0;
  glm::vec3 color = {1.0f, 1.0f, 1.0f};
  bool hasTexture = false;

  UIElement(std::string name, UIHitbox box, glm::vec3 color,
            std::function<void()> cb);
  UIElement(std::string name, UIHitbox box, GLuint texID,
            std::function<void()> cb);
};

class UIManager {
private:
  std::map<UIHitbox, std::unique_ptr<UIElement>> m_elements;

public:
  void add_element(std::string name, UIHitbox box, glm::vec3 color,
                   std::function<void()> cb);
  void add_element(std::string name, UIHitbox box, GLuint texID,
                   std::function<void()> cb);

  bool handle_click(double mouseX, double mouseY);

  void render(const Shader &uiShader, int windowWidth, int windowHeight);
};
