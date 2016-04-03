uniform samplerCube texDiffuse;
uniform lowp float dynamicRangeFactor;

varying mediump vec3 texCoord;

void main()
{
  gl_FragColor = textureCube(texDiffuse, texCoord);
  // Alpha component has the strength of color. it has (255 / strength).
  // We can restore the actual color by multiply (1.0 / alpha).
  // Please see 'default.frag', 'hdrdiff.frag' and 'applyhdr.frag'.
  gl_FragColor.rgb *= dynamicRangeFactor / gl_FragColor.a;
  gl_FragColor.a = 1.0;
}
