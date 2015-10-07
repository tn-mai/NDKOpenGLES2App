attribute highp   vec3 vPosition;
attribute mediump vec4 vTexCoord01;

uniform mat4 matProjection;
uniform mediump vec4 unitTexCoord;

varying vec4 texCoord[2];

void main()
{
  mediump vec4 offset0 = vec4(-1.0, -1.0, 1.0, -1.0) * unitTexCoord.zwzw;
  mediump vec4 offset1 = vec4(-1.0, 1.0, 1.0, 1.0) * unitTexCoord.zwzw;
  mediump vec2 coord = SCALE_TEXCOORD(vTexCoord01.xy) * unitTexCoord.xy;
  texCoord[0] = coord.xyxy + offset0;
  texCoord[1] = coord.xyxy + offset1;

  gl_Position = matProjection * vec4(vPosition, 1);
}
