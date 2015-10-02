attribute highp   vec3 vPosition;
attribute mediump vec4 vTexCoord01;

uniform mat4 matProjection;

varying vec4 texCoord[2];

void main()
{
  const mediump vec4 unitTexCoord = vec4(MAIN_RENDERING_PATH_WIDTH / FBO_MAIN_WIDTH, MAIN_RENDERING_PATH_HEIGHT / FBO_MAIN_HEIGHT, 1.0 / FBO_MAIN_WIDTH, 1.0 / FBO_MAIN_HEIGHT);
  const mediump vec4 offset0 = vec4(-1.0, -1.0, 1.0, -1.0) * unitTexCoord.zwzw;
  const mediump vec4 offset1 = vec4(-1.0, 1.0, 1.0, 1.0) * unitTexCoord.zwzw;
  mediump vec2 coord = SCALE_TEXCOORD(vTexCoord01.xy) * unitTexCoord.xy;
  texCoord[0] = coord.xyxy + offset0;
  texCoord[1] = coord.xyxy + offset1;

  gl_Position = matProjection * vec4(vPosition, 1);
}
