#version 100

uniform sampler2D texDiffuse;
uniform mediump vec4 unitTexCoord;

varying mediump vec2 texCoord;

void main()
{
  mediump vec2 coord = texCoord * unitTexCoord.xy;
  lowp vec3 t0 = texture2D(texDiffuse, coord + vec2(-1.0, -1.0) * unitTexCoord.zw).rgb;
  lowp vec3 t1 = texture2D(texDiffuse, coord + vec2( 1.0, -1.0) * unitTexCoord.zw).rgb;
  lowp vec3 t2 = texture2D(texDiffuse, coord + vec2(-1.0,  1.0) * unitTexCoord.zw).rgb;
  lowp vec3 t3 = texture2D(texDiffuse, coord + vec2( 1.0,  1.0) * unitTexCoord.zw).rgb;
  highp vec3 tmp = t0 + t1 + t2 + t3;
  gl_FragColor = vec4(tmp * (1.0 / 4.0), 1.0);
}
