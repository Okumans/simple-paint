#version 450 core
out vec4 FragColor;
in vec2 WorldPos;

uniform float u_zoom;
uniform vec3 u_gridColor = vec3(0.3);

void main() {
  float size = 1.0;

  vec2 grid = abs(fract(WorldPos / size - 0.5) - 0.5) / (fwidth(WorldPos) / size);
  float line = min(grid.x, grid.y);

  float mask = 1.0 - smoothstep(0.0, 1.5, line);

  if (mask < 0.1) discard;

  FragColor = vec4(u_gridColor, mask * 0.35);
}
