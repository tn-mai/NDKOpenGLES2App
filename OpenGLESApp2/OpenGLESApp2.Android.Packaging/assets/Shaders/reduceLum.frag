uniform sampler2D texDiffuse;

varying mediump vec4 texCoord[2];

#define invLDRRange (255.0/240.0)
#if 1
// HDTV with BT.709
#define luminance(tex) dot(tex, vec4(0.2126, 0.7152, 0.0722, 0.0) * invLDRRange)
#else
// SDTV with BT.601
#define luminance(tex) dot(tex, vec4(0.299, 0.587, 0.114, 0.0) * invLDRRange)
#endif

void main()
{
  gl_FragColor = texture2D(texDiffuse, texCoord[0].xy);
  lowp float lum = luminance(gl_FragColor);

  lowp vec4 tmp = texture2D(texDiffuse, texCoord[0].zw);
  lum = max(lum, luminance(tmp));
  gl_FragColor += tmp;

  tmp = texture2D(texDiffuse, texCoord[1].xy);
  lum = max(lum, luminance(tmp));
  gl_FragColor += tmp;

  tmp = texture2D(texDiffuse, texCoord[1].zw);
  lum = max(lum, luminance(tmp));
  gl_FragColor += tmp;

  gl_FragColor *= (1.0 / 4.0);
  gl_FragColor.a = lum;
}
