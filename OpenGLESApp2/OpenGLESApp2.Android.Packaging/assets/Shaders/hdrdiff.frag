uniform sampler2D texDiffuse;

varying mediump vec2 texCoord;

#if 1
// HDTV with BT.709
#define luminance(tex) dot(tex, vec4(0.2126, 0.7152, 0.0722, 0.0));
#else
// SDTV with BT.601
#define luminance(tex) dot(tex, vec4(0.299, 0.587, 0.114, 0.0));
#endif

#if 1
#define threshold 0.75
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
  const mediump vec2 scale = vec2(1.0 / MAIN_RENDERING_PATH_WIDTH / 4.0, 1.0 / MAIN_RENDERING_PATH_HEIGHT / 4.0);

  lowp vec4 tex0 = texture2D(texDiffuse, texCoord + vec2(-1.0, -1.0) * scale);
  HDRFactor(gl_FragColor, tex0, =, lum_0, lum2_0);

  lowp vec4 tex1 = texture2D(texDiffuse, texCoord + vec2( 1.0, -1.0) * scale);
  HDRFactor(gl_FragColor, tex1, +=, lum_1, lum2_1);

  lowp vec4 tex2 = texture2D(texDiffuse, texCoord + vec2(-1.0,  1.0) * scale);
  HDRFactor(gl_FragColor, tex2, +=, lum_2, lum2_2);

  lowp vec4 tex3 = texture2D(texDiffuse, texCoord + vec2( 1.0,  1.0) * scale);
  HDRFactor(gl_FragColor, tex3, +=, lum_3, lum2_3);

  gl_FragColor *= 1.0 / 4.0;
  gl_FragColor.a = 1.0;
}
