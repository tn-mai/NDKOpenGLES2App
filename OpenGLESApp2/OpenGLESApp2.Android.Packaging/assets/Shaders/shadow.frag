varying mediump vec3 depth3;

void main()
{
  const highp float coef = 1.0 / 256.0;
  highp float depth = dot(depth3, vec3(1.0, coef, coef * coef));

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
}
