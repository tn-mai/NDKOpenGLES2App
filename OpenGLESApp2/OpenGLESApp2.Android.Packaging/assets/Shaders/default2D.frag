uniform sampler2D texDiffuse;

varying mediump vec4 texCoord;

void main()
{
  gl_FragColor = texture2D(texDiffuse, texCoord.xy);
}
