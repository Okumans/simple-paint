#version 450 core

layout(location = 0) out vec4 FinalColor;

in vec3 FragColor;
in vec2 TexCoords;
in float vThickness;
in float vTotalLength;

uniform float u_alpha = 1.0;

void main() {
  float radius = vThickness * 0.5;

  float dx = (TexCoords.x - 0.5) * vThickness;

  float dy = 0.0;
  if (TexCoords.y < 0.0) {
    dy = TexCoords.y;
  } else if (TexCoords.y > vTotalLength) {
    dy = TexCoords.y - vTotalLength;
  }

  float dist = sqrt(dx * dx + dy * dy);

  // Soft anti-aliased edges
  float edge_softness = fwidth(dist);
  float alpha = 1.0 - smoothstep(radius - edge_softness, radius, dist);

  if (alpha <= 0.0) discard;

  FinalColor = vec4(FragColor, alpha * u_alpha);
}
