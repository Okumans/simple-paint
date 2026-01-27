#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in float aRotateSpeed;
layout(location = 3) in float aBrightness;

uniform float u_time;
uniform float u_aspect;
uniform vec2 u_mouse_pos;
uniform float u_scale;

out vec3 initialColor;
out float brightness;

void main()
{
  float breath = 1.0 + sin(u_time * 1.0 + aRotateSpeed) * 0.15;

  // 1. Rotation Logic
  float s = sin(u_time * aRotateSpeed);
  float c = cos(u_time * aRotateSpeed);
  vec2 rotatedPosition;
  rotatedPosition.x = aPos.x * c - aPos.y * s;
  rotatedPosition.y = aPos.x * s + aPos.y * c;

  // 2. Apply Breath & Aspect Ratio to the SHAPE only
  vec2 shapePos = rotatedPosition * breath * u_scale;
  shapePos.x /= u_aspect; // Fix the oval shape here

  // 3. Add Mouse Position (Screen Space Translation)
  // Since u_mouse_pos is already -1 to 1, we just add it directly.
  vec2 finalPos = shapePos + u_mouse_pos;

  // 4. Output (Do NOT divide by u_aspect again)
  gl_Position = vec4(finalPos.x, finalPos.y, aPos.z, 1.0);

  initialColor = aColor;
  brightness = aBrightness;
}
