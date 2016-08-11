precision mediump float;

uniform lowp vec4 fontOutlineInfo;
uniform mediump vec4 fontDropShadowInfo;

uniform sampler2D texDiffuse;

varying mediump vec2 texCoord;
varying lowp vec4 color;

void main()
{
  lowp vec4 tex = texture2D(texDiffuse, texCoord);

#if 1
//  lowp vec4 outlineScale = vec4(0.45, 0.5, 0.65, 0.75);
//  lowp vec4 outlineScale = vec4(0.6, 0.7, -1.0, 1.0);
  // Outline
  gl_FragColor.rgb = smoothstep(fontOutlineInfo.z, fontOutlineInfo.w, tex.g) * color.rgb;
  gl_FragColor.a = smoothstep(fontOutlineInfo.x, fontOutlineInfo.y, tex.g); // Soft edge
#else
  gl_FragColor.rgb = color.rgb;
  //gl_FragColor.a = tex.g * color.a; // default
  //gl_FragColor.a = step(0.75, tex.g) * color.a; // Hard edge
  gl_FragColor.a = smoothstep(0.6, 0.7, tex.g); // Soft edge
#endif

#if 1
  // Dropshadow
//  mediump vec4 dropShadowInfo = vec4(-3.0 / 512.0, 3.0 / 1024.0, 0.6, 0.7); // offset x, offset y, alpha
//  mediump vec4 dropShadowInfo = vec4(0.0 / 512.0, 0.0 / 1024.0, 0.6, 0.7); // offset x, offset y, alpha
  lowp vec4 tex2 = texture2D(texDiffuse, texCoord + fontDropShadowInfo.xy);
  tex2.a = smoothstep(fontDropShadowInfo.z, fontDropShadowInfo.w, tex2.g);
  gl_FragColor.rgb = mix(vec3(0, 0, 0), gl_FragColor.rgb, clamp(gl_FragColor.a + (1.0 - tex2.a), 0.0, 1.0));
  gl_FragColor.a = max(tex2.a * 0.5, gl_FragColor.a);
#endif

  gl_FragColor.a *= color.a;
}
