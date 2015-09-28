#version 330 core

uniform sampler2D texSampler;

in vec4 col;
in vec2 texCoord;

out vec4 fColor;

void main(void)
{
  vec4 texColor = texture2D(texSampler, texCoord);
  fColor = col * texColor;
}
