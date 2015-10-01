uniform sampler2D texShadow;

varying mediump vec4 texCoord[2];

void main()
{
  gl_FragColor = texture2D(texShadow, texCoord[0].xy);
  gl_FragColor += texture2D(texShadow, texCoord[0].zw);
  gl_FragColor += texture2D(texShadow, texCoord[1].xy);
  gl_FragColor += texture2D(texShadow, texCoord[1].zw);
  gl_FragColor *= (1.0 / 4.0);

  const mediump float coef = 1.0 / 255.0;
  gl_FragColor.xz += gl_FragColor.yw * coef;
  gl_FragColor.yw = fract(gl_FragColor.xz * 255.0);
  gl_FragColor.xz -= gl_FragColor.yw * coef;
}
