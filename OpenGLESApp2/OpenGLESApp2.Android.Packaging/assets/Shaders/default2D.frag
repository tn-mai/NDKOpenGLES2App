uniform lowp vec4 materialColor;
uniform sampler2D texDiffuse;

varying mediump vec2 texCoord;

void main()
{
  gl_FragColor = texture2D(texDiffuse, texCoord) * materialColor;
}
