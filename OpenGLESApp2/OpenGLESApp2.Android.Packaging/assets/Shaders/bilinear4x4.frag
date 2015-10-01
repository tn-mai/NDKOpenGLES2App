uniform sampler2D texShadow;

varying mediump vec2 texCoord;

highp vec2 frexp(highp float x)
{
	highp float e = ceil(log2(x));
	return vec2(x * exp2(-e), e * (-1.0 / 255.0));
}

highp float ldexp(lowp vec2 v)
{
	return v.x * exp2(v.y * (-255.0));
}

void main()
{
  const mediump float coef = 1.0 / 255.0;
  const mediump vec2 textureSize = vec2(FBO_MAIN_WIDTH, FBO_MAIN_HEIGHT);
  const mediump vec2 sourceSize = vec2(SHADOWMAP_MAIN_WIDTH, SHADOWMAP_MAIN_HEIGHT);
  mediump vec2 coord = texCoord * (sourceSize / textureSize);
#if 0
  mediump float x, y;
  highp vec2 acc = vec2(0.0, 0.0);
  for (x = -1.5; x <= 1.5; x += 1.0) {
	for (y = -1.5; y <= 1.5; y += 1.0) {
	  lowp vec4 tex = texture2D(texShadow, coord + vec2(x, y) * (vec2(1.0, 1.0) / textureSize));
	  highp vec2 tmp;
	  tmp.x = dot(tex, vec4(1.0, 1.0 / 255.0, 1.0 / 255.0 / 255.0, 1.0 / 255.0 / 255.0 / 255.0))
		//tmp.x = tex.x + tex.y * (1.0 / 255.0);
		tmp.y = tmp.x * tmp.x;// ldexp(tex.zw);
	  highp float scaleFactor = 1.0;// (1.0 / (abs(x) + 0.5)) * (1.0 / (abs(y) + 0.5));
	  acc += tmp * scaleFactor;
	}
  }
  acc *= 1.0 / 16.0;
  //gl_FragColor.xy = frexp(acc.x);
  gl_FragColor.x = acc.x;
  gl_FragColor.y = fract(acc.x * 255.0);
  gl_FragColor.x -= gl_FragColor.y * (1.0 / 255.0);
  gl_FragColor.zw = frexp(acc.y);
#else
  const mediump vec2 off = vec2(1.0, 1.0) / textureSize;
  lowp vec4 t0 = texture2D(texShadow, coord + vec2(-off.x, -off.y));
  lowp vec4 t1 = texture2D(texShadow, coord + vec2( off.x, -off.y));
  lowp vec4 t2 = texture2D(texShadow, coord + vec2(-off.x,  off.y));
  lowp vec4 t3 = texture2D(texShadow, coord + vec2( off.x,  off.y));
  highp vec4 tmp = (t0 + t1 + t2 + t3) * (1.0 / 4.0);

  gl_FragColor.xz = tmp.xz + tmp.yw * coef;
  gl_FragColor.z += gl_FragColor.x * gl_FragColor.x * 0.25;
  gl_FragColor.yw = fract(gl_FragColor.xz * 255.0);
  gl_FragColor.xz -= gl_FragColor.yw * coef;
#endif
}
