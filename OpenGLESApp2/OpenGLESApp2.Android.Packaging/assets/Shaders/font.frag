precision mediump float;

uniform sampler2D texDiffuse;

varying mediump vec2 texCoord;
varying lowp vec4 color;

void main()
{
  gl_FragColor = texture2D(texDiffuse, texCoord) * color;
}
