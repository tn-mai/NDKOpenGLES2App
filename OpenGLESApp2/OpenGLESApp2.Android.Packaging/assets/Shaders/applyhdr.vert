attribute highp   vec3 vPosition;
attribute mediump vec4 vTexCoord01;

uniform mat4 matProjection;

varying mediump vec4 texCoord; // xy for main. zw for other.

void main()
{
  const mediump vec2 scaleMain = vec2(480.0 / 512.0, 800.0 / 800.0);
  texCoord.zw = vTexCoord01.xy * (1.0 / 65535.0);
  texCoord.xy = texCoord.zw * scaleMain;
  gl_Position = matProjection * vec4(vPosition, 1);
}
