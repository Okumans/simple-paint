#version 330 core
#define TWO_PI 6.28318530718

in vec3 initialColor;
in float brightness;
out vec4 FragColor;
uniform float u_time;

void main()
{
  float intensity = length(initialColor) / 1.732;
  float glow = pow(intensity, 2.0);
  float red = brightness * (sin(u_time + initialColor.r * TWO_PI) *
        0.5 + 0.5);
  float green = brightness * (sin(u_time + initialColor.g * TWO_PI) *
        0.5 + 0.5);
  float blue = brightness * (cos(u_time + initialColor.b * TWO_PI) *
        0.5 + 0.5);
  FragColor = vec4(red * glow, green * glow, blue * glow, 1.0);
}
