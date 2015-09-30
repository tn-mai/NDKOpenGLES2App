#version 100

uniform sampler2D texSource[4]; // 0:main color, 1:1/4 color, 2:1/16 hdr factor, 3:1/64 hdr factor.

varying mediump vec4 texCoord; // xy for main. zw for other.

mediump float luminance(lowp vec4 tex)
{
#if 1
  // HDTV with BT.709
  return dot(tex, vec4(0.2126, 0.7152, 0.0722, 0.0));
#else
  // SDTV with BT.601
  return dot(tex, vec4(0.299, 0.587, 0.114, 0.0));
#endif
}

mediump vec4 HDRFactor(lowp vec4 tex)
{
  const mediump float threshold = 0.75;
  mediump float lum = luminance(tex);
  mediump float lum2 = max(lum - threshold, 0.0);
  return tex * (lum2 / lum);
}

void main()
{
  lowp vec4 t0 = texture2D(texSource[0], texCoord.xy);
  lowp vec4 t1 = HDRFactor(texture2D(texSource[1], texCoord.zw));
  lowp vec4 t2 = texture2D(texSource[2], texCoord.zw);
  lowp vec4 t3 = texture2D(texSource[3], texCoord.zw);
  gl_FragColor = t0 + t1 + t2 + t3;
  gl_FragColor.a = t0.a;
}
