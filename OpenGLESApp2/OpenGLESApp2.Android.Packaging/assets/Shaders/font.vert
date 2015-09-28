#version 100

precision mediump float;

attribute mediump vec3 vPosition;
attribute mediump vec4 vTexCoord01;

varying mediump vec2 texCoord;

void main()
{
	texCoord = vTexCoord01.xy * (1.0 / 65535.0);
	gl_Position = vec4(vPosition, 1);
}
