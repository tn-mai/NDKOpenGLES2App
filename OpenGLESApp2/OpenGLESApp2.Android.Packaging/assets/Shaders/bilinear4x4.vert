attribute highp   vec3 vPosition;
attribute mediump vec4 vTexCoord01;

uniform mat4 matProjection;

varying mediump vec4 texCoord[2];

void main()
{
  const mediump vec2 textureSize = vec2(FBO_MAIN_WIDTH, FBO_MAIN_HEIGHT);
  const mediump vec2 sourceSize = vec2(SHADOWMAP_MAIN_WIDTH, SHADOWMAP_MAIN_HEIGHT);
  mediump vec2 coord = SCALE_TEXCOORD(vTexCoord01.xy) * (sourceSize / textureSize);

  const mediump float step = 2.0 / 3.0;
  const mediump vec4 offset0 = (vec4(-1.0, -1.0,  1.0, -1.0) * step) / textureSize.xyxy;
  const mediump vec4 offset1 = (vec4(-1.0,  1.0,  1.0,  1.0) * step) / textureSize.xyxy;
  texCoord[0] = coord.xyxy + offset0;
  texCoord[1] = coord.xyxy + offset1;

  gl_Position = matProjection * vec4(vPosition, 1);
}
