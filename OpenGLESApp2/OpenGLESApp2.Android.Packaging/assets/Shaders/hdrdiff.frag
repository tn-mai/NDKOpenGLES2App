uniform sampler2D texDiffuse;
uniform lowp vec2 dynamicRangeFactor;

varying mediump vec2 texCoord;

void main()
{
  gl_FragColor = texture2D(texDiffuse, texCoord.xy);
  lowp float lum2 = max(gl_FragColor.a - dynamicRangeFactor.x, 0.0);
  // Diffuse has the compressed range 0-2 to 0-1, because keep the HDR
  // luminance. Thus we must restore to LDR range by multiply 2, and we want
  // to make the beautiful bloom. so we need to multiply before to apply the
  // blur filter. The tone is noticeable if multiply after the filter.
  // Note that the base color multiplication is applying in 'applyhdr.frag'.
  gl_FragColor.rgb *= lum2 * dynamicRangeFactor.y;
}
