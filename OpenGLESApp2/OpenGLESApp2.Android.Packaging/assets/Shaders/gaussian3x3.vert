attribute highp   vec3 vPosition;
attribute mediump vec4 vTexCoord01;

uniform mat4 matProjection;

varying vec2 texCoord;

void main()
{
  texCoord = SCALE_TEXCOORD(vTexCoord01.xy);
  gl_Position = matProjection * vec4(vPosition, 1);
}
