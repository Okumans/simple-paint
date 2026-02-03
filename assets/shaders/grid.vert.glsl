#version 450 core
layout(location = 0) in vec2 aPos;

out vec2 WorldPos;

uniform mat4 u_projection;
uniform mat4 u_model;

void main() {
  vec4 pos = u_model * vec4(aPos, 0.0, 1.0);
  WorldPos = pos.xy;
  gl_Position = u_projection * pos;
}
