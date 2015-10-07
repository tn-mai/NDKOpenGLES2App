uniform sampler2D texDiffuse;

varying mediump vec4 texCoord[2];

void main()
{
  gl_FragColor = texture2D(texDiffuse, texCoord[0].xy);
  gl_FragColor += texture2D(texDiffuse, texCoord[0].zw);
  gl_FragColor += texture2D(texDiffuse, texCoord[1].xy);
  gl_FragColor += texture2D(texDiffuse, texCoord[1].zw);
  gl_FragColor *= (1.0 / 4.0);
}
