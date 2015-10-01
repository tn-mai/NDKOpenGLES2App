attribute highp   vec3 vPosition;
attribute mediump vec4 vTexCoord01;

uniform mat4 matProjection;

varying mediump vec4 texCoord[2];

void main()
{
  const mediump vec2 textureSize = vec2(SHADOWMAP_SUB_WIDTH, SHADOWMAP_SUB_HEIGHT);
  const mediump vec4 offset0 = vec4(-0.5, -0.5, 0.5, -0.5) / textureSize.xyxy;
  const mediump vec4 offset1 = vec4(-0.5,  0.5, 0.5,  0.5) / textureSize.xyxy;
  mediump vec2 coord = SCALE_TEXCOORD(vTexCoord01.xy);
  texCoord[0] = coord.xyxy + offset0;
  texCoord[1] = coord.xyxy + offset1;

  gl_Position = matProjection * vec4(vPosition, 1);
}
