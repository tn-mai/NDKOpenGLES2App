#version 100

attribute highp   vec3 vPosition;
attribute mediump vec4 vTexCoord01;

uniform mat4 matProjection;

varying vec2 texCoord;

void main()
{
  texCoord = vTexCoord01.xy * (1.0 / 65535.0);
  gl_Position = matProjection * vec4(vPosition, 1);
}
