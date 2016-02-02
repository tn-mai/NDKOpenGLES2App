precision mediump float;

uniform sampler2D texDiffuse;

varying mediump vec2 texCoord;
varying lowp vec4 color;

void main()
{
  lowp vec4 tex = texture2D(texDiffuse, texCoord);
  /*
  // Outline
  if (tex.g <= 0.75) {
	gl_FragColor.rgb = vec3(0.0, 0.0, 0.0);
  } else {
	gl_FragColor.rgb = color.rgb;
  }
  */
  gl_FragColor.rgb = color.rgb;
  //gl_FragColor.a = tex.g * color.a; // default
  //gl_FragColor.a = step(0.5, tex.g) * color.a; // Hard edge
  gl_FragColor.a = smoothstep(0.5, 1.0, tex.g) * color.a; // Soft edge
}
