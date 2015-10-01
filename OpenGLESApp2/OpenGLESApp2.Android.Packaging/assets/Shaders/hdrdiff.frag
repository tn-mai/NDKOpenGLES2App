uniform sampler2D texDiffuse;

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
  const mediump vec2 scale = vec2(1.0 / MAIN_RENDERING_PATH_WIDTH / 4.0, 1.0 / MAIN_RENDERING_PATH_HEIGHT / 4.0);
  lowp vec3 t0 = HDRFactor(texture2D(texDiffuse, texCoord + vec2(-1.0, -1.0) * scale).rgb);
  lowp vec3 t1 = HDRFactor(texture2D(texDiffuse, texCoord + vec2( 1.0, -1.0) * scale).rgb);
  lowp vec3 t2 = HDRFactor(texture2D(texDiffuse, texCoord + vec2(-1.0,  1.0) * scale).rgb);
  lowp vec3 t3 = HDRFactor(texture2D(texDiffuse, texCoord + vec2( 1.0,  1.0) * scale).rgb);
  mediump vec3 tmp = t0 + t1 + t2 + t3;
  tmp *= 1.0 / 4.0;
  gl_FragColor = vec4(tmp, 1.0);
}
