#include "geometry.h"

void draw_quad() {
  static GLuint quadVAO = 0;
  static GLuint quadVBO = 0;

  if (quadVAO == 0) {
    QuadVertex vertices[] = {
        {{0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 1.0f}, {0.0f, 1.0f}},
        {{1.0f, 1.0f}, {1.0f, 1.0f}}, {{0.0f, 0.0f}, {0.0f, 0.0f}},
        {{1.0f, 1.0f}, {1.0f, 1.0f}}, {{1.0f, 0.0f}, {1.0f, 0.0f}}};

    glCreateVertexArrays(1, &quadVAO);
    glCreateBuffers(1, &quadVBO);

    glNamedBufferStorage(quadVBO, sizeof(vertices), vertices, 0);
    glVertexArrayVertexBuffer(quadVAO, 0, quadVBO, 0, 4 * sizeof(float));

    // Attribute 0: Position
    glEnableVertexArrayAttrib(quadVAO, 0);
    glVertexArrayAttribFormat(quadVAO, 0, 2, GL_FLOAT, GL_FALSE,
                              offsetof(QuadVertex, pos));
    glVertexArrayAttribBinding(quadVAO, 0, 0);

    // Attribute 1: UV
    glEnableVertexArrayAttrib(quadVAO, 1);
    glVertexArrayAttribFormat(quadVAO, 1, 2, GL_FLOAT, GL_FALSE,
                              offsetof(QuadVertex, uv));
    glVertexArrayAttribBinding(quadVAO, 1, 0);
  }

  // Still need to bind the VAO to draw
  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}
