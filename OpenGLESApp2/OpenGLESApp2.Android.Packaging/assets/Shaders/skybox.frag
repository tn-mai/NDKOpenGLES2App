uniform samplerCube texDiffuse;

varying mediump vec3 texCoord;

void main()
{
  gl_FragColor = textureCube(texDiffuse, texCoord) * 0.95;
}
