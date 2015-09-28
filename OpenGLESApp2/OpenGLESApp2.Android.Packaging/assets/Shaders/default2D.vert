#version 100

precision mediump float;

attribute mediump vec3 vPosition;
attribute mediump vec4 vTexCoord01;

uniform mat4 matView;
uniform mat4 matProjection;

varying mediump vec4 texCoord;

void main()
{
	texCoord = vTexCoord01 * (1.0 / 65535.0);
	gl_Position = matProjection * matView * vec4(vPosition, 1);
}
