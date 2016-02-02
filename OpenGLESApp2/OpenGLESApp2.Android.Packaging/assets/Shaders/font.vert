precision mediump float;

attribute mediump vec2 vPosition;
attribute mediump vec2 vTexCoord01;
attribute lowp vec4 vColor;

varying mediump vec2 texCoord;
varying lowp vec4 color;

void main()
{
	texCoord = vTexCoord01.xy;
	color = vColor;
	gl_Position = vec4(vPosition.x, vPosition.y, 0, 1);
}
