attribute highp   vec3 vPosition;
attribute mediump vec4 vTexCoord01;

uniform mat4 matProjection;

varying mediump vec4 texCoord[2];

void main()
{
  const mediump vec4 scale = vec4(1.0 / MAIN_RENDERING_PATH_WIDTH / 4.0, 1.0 / MAIN_RENDERING_PATH_HEIGHT / 4.0, 1.0 / MAIN_RENDERING_PATH_WIDTH / 4.0, 1.0 / MAIN_RENDERING_PATH_HEIGHT / 4.0);
  const mediump vec4 offset0 = vec4(-1.0, -1.0,  1.0, -1.0) * scale;
  const mediump vec4 offset1 = vec4(-1.0,  1.0,  1.0,  1.0) * scale;

  mediump vec4 tc;
  tc.xy = SCALE_TEXCOORD(vTexCoord01.xy);
  tc.zw = tc.xy;
  texCoord[0] = tc + offset0;
  texCoord[1] = tc + offset1;
  gl_Position = matProjection * vec4(vPosition, 1);
}
