#version 100

varying mediump vec3 depth3;

void main()
{
  highp float depth = dot(depth3, vec3(1.0, 1.0 / 256.0, 1.0 / 256.0 / 256.0));

  const highp float coef = 1.0 / 255.0;

  gl_FragColor.x = depth;
  gl_FragColor.z = depth * depth;
  gl_FragColor.yw = fract(gl_FragColor.xz * 255.0);
  gl_FragColor.xz -= gl_FragColor.yw * coef;
}
