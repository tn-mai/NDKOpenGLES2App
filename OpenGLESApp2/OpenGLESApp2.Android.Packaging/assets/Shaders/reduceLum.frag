uniform sampler2D texDiffuse;

varying mediump vec4 texCoord[2];

#define invLDRRange 1.0//(255.0/240.0)
#if 1
// HDTV with BT.709
#define luminance(tex) dot(tex, vec3(0.2126, 0.7152, 0.0722) * invLDRRange)
#else
// SDTV with BT.601
#define luminance(tex) dot(tex, vec3(0.299, 0.587, 0.114) * invLDRRange)
#endif

void main()
{
  gl_FragColor.rgb = texture2D(texDiffuse, texCoord[0].xy).rgb;
  lowp float lum = luminance(gl_FragColor.rgb);

  lowp vec3 tmp = texture2D(texDiffuse, texCoord[0].zw).rgb;
  lum = max(lum, luminance(tmp));
  gl_FragColor.rgb += tmp;

  tmp = texture2D(texDiffuse, texCoord[1].xy).rgb;
  lum = max(lum, luminance(tmp));
  gl_FragColor.rgb += tmp;

  tmp = texture2D(texDiffuse, texCoord[1].zw).rgb;
  lum = max(lum, luminance(tmp));
  gl_FragColor.rgb += tmp;

  gl_FragColor.rgb *= (1.0 / 4.0);
  gl_FragColor.a = lum;
}
