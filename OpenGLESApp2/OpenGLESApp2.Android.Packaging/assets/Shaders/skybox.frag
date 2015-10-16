uniform samplerCube texDiffuse;

varying mediump vec3 texCoord;

void main()
{
  gl_FragColor.rgb = textureCube(texDiffuse, texCoord).rgb * 0.75;
  gl_FragColor.a = 1.0;
}
