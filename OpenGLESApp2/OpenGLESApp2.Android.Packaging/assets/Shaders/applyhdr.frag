#version 100

uniform sampler2D texSource[4]; // 0:main color, 1:1/4 color, 2:1/16 hdr factor, 3:1/64 hdr factor.

varying mediump vec2 texCoord;

mediump float luminance(lowp vec3 rgb)
{
#if 1
  // HDTV with BT.709
  return dot(rgb, vec3(0.2126, 0.7152, 0.0722));
#else
  // SDTV with BT.601
  return dot(rgb, vec3(0.299, 0.587, 0.114));
#endif
}

mediump vec3 HDRFactor(lowp vec3 rgb)
{
  const mediump float threshold = 0.75;
  mediump float lum = luminance(rgb);
  mediump float lum2 = max(lum - threshold, 0.0);
  return rgb * (lum2 / lum);
}

void main()
{
  const mediump vec2 scaleMain = vec2(480.0 / 512.0, 800.0 / 800.0);

  lowp vec4 t0 = texture2D(texSource[0], texCoord * scaleMain);
  lowp vec3 t1 = HDRFactor(texture2D(texSource[1], texCoord).rgb);
  lowp vec3 t2 = texture2D(texSource[2], texCoord).rgb;
  lowp vec3 t3 = texture2D(texSource[3], texCoord).rgb;
  mediump vec3 tmp = t0.rgb * 0.95 + (t1 * 1.5 + t2 * 1.25 + t3);
  gl_FragColor = vec4(tmp, t0.a);
}
