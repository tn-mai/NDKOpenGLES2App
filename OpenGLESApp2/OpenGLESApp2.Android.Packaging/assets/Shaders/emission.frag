uniform mediump vec4 materialColor;
uniform lowp float dynamicRangeFactor;
uniform sampler2D texDiffuse;

varying mediump vec4 texCoord;

void main(void)
{
  const mediump float threshold = 0.25;
  mediump vec3 col = texture2D(texDiffuse, texCoord.xy).rgb * materialColor.rgb * ((1.0 / threshold) / dynamicRangeFactor);
  gl_FragColor = vec4(col, materialColor.a);
}
