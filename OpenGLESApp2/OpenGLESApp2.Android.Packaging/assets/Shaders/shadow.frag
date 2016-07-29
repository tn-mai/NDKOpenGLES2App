varying highp float depth;

#ifdef USE_ALPHA_TEST_IN_SHADOW_RENDERING
uniform sampler2D texDiffuse;
varying mediump vec4 texCoord;
#endif // USE_ALPHA_TEST_IN_SHADOW_RENDERING

void main()
{
  const highp float coef = 1.0 / 256.0;

#ifdef USE_ALPHA_TEST_IN_SHADOW_RENDERING
  if (texture2D(texDiffuse, texCoord.xy).a < 0.25) {
	discard;
  } else {
#endif // USE_ALPHA_TEST_IN_SHADOW_RENDERING

#if 1
	gl_FragColor.x = depth;
	gl_FragColor.y = fract(gl_FragColor.x * 256.0);
	gl_FragColor.z = fract(gl_FragColor.y * 256.0);
	gl_FragColor.w = fract(gl_FragColor.z * 256.0);
	gl_FragColor.xyz -= gl_FragColor.yzw * coef;
#else
	gl_FragColor.x = depth;
	gl_FragColor.z = depth * depth;
	gl_FragColor.yw = fract(gl_FragColor.xz * 255.0);
	gl_FragColor.xz -= gl_FragColor.yw * coef;
#endif

#ifdef USE_ALPHA_TEST_IN_SHADOW_RENDERING
  }
#endif // USE_ALPHA_TEST_IN_SHADOW_RENDERING
}
