uniform sampler2D texShadow;

varying mediump vec2 texCoord;

void main()
{
  const mediump float coef = 1.0 / 255.0;
  const mediump vec2 textureSize = vec2(SHADOWMAP_SUB_WIDTH, SHADOWMAP_SUB_HEIGHT);
  const mediump float width = 1.0 / SHADOWMAP_SUB_WIDTH;
  const mediump vec2 off = vec2(0.5, 0.5) / textureSize;
  lowp vec4 t0 = texture2D(texShadow, texCoord + vec2(-off.x, -off.y));
  lowp vec4 t1 = texture2D(texShadow, texCoord + vec2( off.x, -off.y));
  lowp vec4 t2 = texture2D(texShadow, texCoord + vec2(-off.x,  off.y));
  lowp vec4 t3 = texture2D(texShadow, texCoord + vec2( off.x,  off.y));
  highp vec4 tmp = (t0 + t1 + t2 + t3) * (1.0 / 4.0);

  gl_FragColor.xz = tmp.xz + tmp.yw * coef;
  gl_FragColor.yw = fract(gl_FragColor.xz * 255.0);
  gl_FragColor.xz -= gl_FragColor.yw * coef;
}
