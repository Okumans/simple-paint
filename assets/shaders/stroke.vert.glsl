#version 450 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aUv;
layout(location = 3) in float aTotalLength;

out vec3 FragColor;
out vec2 TexCoords;
out float vTotalLength;

uniform mat4 u_projection;

void main() {
  // Apply the projection matrix to the vertex position
  gl_Position = u_projection * vec4(aPos, 0.0, 1.0);

  // Pass the color to the fragment shader
  FragColor = aColor;

  TexCoords = aUv;

  vTotalLength = aTotalLength;
}
