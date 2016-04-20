uniform sampler2D texShadow;

varying mediump vec4 texCoord[2];

void main()
{
  gl_FragColor = texture2D(texShadow, texCoord[0].xy);
  gl_FragColor += texture2D(texShadow, texCoord[0].zw);
  gl_FragColor += texture2D(texShadow, texCoord[1].xy);
  gl_FragColor += texture2D(texShadow, texCoord[1].zw);
  gl_FragColor *= (1.0 / 4.0);

  const highp float coef = 1.0 / 256.0;
  gl_FragColor.x = dot(gl_FragColor, vec4(1.0, coef, coef * coef, coef * coef * coef));
  gl_FragColor.y = fract(gl_FragColor.x * 256.0);
  gl_FragColor.z = fract(gl_FragColor.y * 256.0);
  gl_FragColor.w = fract(gl_FragColor.z * 256.0);
  gl_FragColor.xyz -= gl_FragColor.yzw * coef;
}
