attribute mediump vec3 vPosition;
attribute mediump vec4 vTexCoord01;

uniform mat4 matView;
uniform mat4 matProjection;
uniform mediump vec4 unitTexCoord;

varying mediump vec2 texCoord;

void main()
{
	texCoord = SCALE_TEXCOORD(vTexCoord01.xy) * unitTexCoord.xy;
	gl_Position = matProjection * matView * vec4(vPosition, 1);
}
