uniform sampler2D texDiffuse;

varying mediump vec2 texCoord;

void main()
{
  const lowp float threshold = 0.75;
  gl_FragColor = texture2D(texDiffuse, texCoord.xy);
  lowp float lum2 = max(gl_FragColor.a - threshold, 0.0);
  gl_FragColor.rgb *= lum2 / gl_FragColor.a;
}
