#include "ui_manager.h"

#include "glad/gl.h"
#include <glm/gtc/matrix_transform.hpp>

#include "geometry.h"

// UIElement Constructors
UIElement::UIElement(std::string name, UIHitbox box, glm::vec3 color,
                     std::function<void()> cb)
    : name(std::move(name)), hitbox(box), onClick(std::move(cb)), color(color),
      hasTexture(false) {}

UIElement::UIElement(std::string name, UIHitbox box, GLuint texID,
                     std::function<void()> cb)
    : name(std::move(name)), hitbox(box), onClick(std::move(cb)),
      textureID(texID), hasTexture(true) {}

// UIManager Implementations
void UIManager::add_element(std::string name, UIHitbox box, glm::vec3 color,
                            std::function<void()> cb) {
  auto el =
      std::make_unique<UIElement>(std::move(name), box, color, std::move(cb));
  m_elements.emplace(box, std::move(el));
}

void UIManager::add_element(std::string name, UIHitbox box, GLuint texID,
                            std::function<void()> cb) {
  auto el =
      std::make_unique<UIElement>(std::move(name), box, texID, std::move(cb));
  m_elements.emplace(box, std::move(el));
}

bool UIManager::handle_click(double mouseX, double mouseY) {
  auto it_end = m_elements.upper_bound({(float)mouseX, 0, 0, 0});

  auto it = it_end;
  while (it != m_elements.begin()) {
    --it;
    const UIHitbox &box = it->first;

    // Pruning: if the element ends before the mouseX, we can stop
    // (Since we are iterating backwards from X > mouseX)
    if (box.x + box.w < mouseX)
      break;

    if (box.contains(mouseX, mouseY)) {
      it->second->onClick();
      return true;
    }
  }
  return false;
}

void UIManager::render(const Shader &uiShader, int windowWidth,
                       int windowHeight) {
  uiShader.use();

  // Screen-space projection: (0,0) at top-left
  glm::mat4 projection =
      glm::ortho(0.0f, (float)windowWidth, (float)windowHeight, 0.0f);
  uiShader.setMat4("u_projection", projection);

  // Enable blending for icons/transparency
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  for (const auto &pair : m_elements) {
    const UIHitbox &box = pair.first;
    const auto &el = pair.second;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(box.x, box.y, 0.0f));
    model = glm::scale(model, glm::vec3(box.w, box.h, 1.0f));
    uiShader.setMat4("u_model", model);

    uiShader.setBool("u_hasTexture", el->hasTexture);
    if (el->hasTexture) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, el->textureID);
      uiShader.setInt("u_icon", 0);
    } else {
      uiShader.setVec3("u_color", el->color);
    }

    draw_quad();
  }
}
