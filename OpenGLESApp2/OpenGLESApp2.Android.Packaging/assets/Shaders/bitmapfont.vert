#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 2) in vec4 vColor;
layout(location = 3) in vec2 vTexCoord;

out vec4 col;
out vec2 texCoord;

void main()
{
  col = vColor;
  texCoord = vTexCoord;
  gl_Position = vec4(vPosition, 1);
}
