uniform sampler2D texSource[4]; // 0:main color, 1:1/4 color, 2:1/16 hdr factor, 3:1/64 hdr factor.

varying mediump vec4 texCoord; // xy for main. zw for other.

#if 1
// HDTV with BT.709
#define luminance(tex) dot(tex, vec4(0.2126, 0.7152, 0.0722, 0.0));
#else
// SDTV with BT.601
#define luminance(tex) dot(tex, vec4(0.299, 0.587, 0.114, 0.0));
#endif

#define threshold 0.75

#if 1
#define HDRFactor(result, tex, op, var0, var1) \
  mediump float var0 = luminance(tex); \
  mediump float var1 = max(var0 - threshold, 0.0); \
  result op tex * (var1 / var0);
#else
mediump vec4 HDRFactor(lowp vec4 tex)
{
  mediump float lum = luminance(tex);
  mediump float lum2 = max(lum - threshold, 0.0);
  return tex * (lum2 / lum);
}
#endif

void main()
{
  gl_FragColor = texture2D(texSource[0], texCoord.xy);
  lowp vec4 tex = texture2D(texSource[1], texCoord.zw);
  HDRFactor(gl_FragColor, tex, +=, lum, lum2);
  gl_FragColor += texture2D(texSource[2], texCoord.zw);
  gl_FragColor += texture2D(texSource[3], texCoord.zw);
}
