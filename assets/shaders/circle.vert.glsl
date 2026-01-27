#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in float aRotateSpeed;
layout(location = 3) in float aBrightness;

uniform float u_time;
uniform float u_aspect;
out vec3 initialColor;
out float brightness;

void main()
{
  float breath = 1.0 + sin(u_time * 1.0 + aRotateSpeed) * 0.15;
  vec2 animatedPos = aPos.xy * breath;

  float s = sin(u_time * aRotateSpeed);
  float c = cos(u_time * aRotateSpeed);
  vec2 rotatedPosition;
  rotatedPosition.x = animatedPos.x * c - animatedPos.y * s;
  rotatedPosition.y = animatedPos.x * s + animatedPos.y * c;

  gl_Position = vec4(rotatedPosition.x / u_aspect, rotatedPosition.y,
      aPos.z, 1.0);
  initialColor = aColor;
  brightness = aBrightness;
}
