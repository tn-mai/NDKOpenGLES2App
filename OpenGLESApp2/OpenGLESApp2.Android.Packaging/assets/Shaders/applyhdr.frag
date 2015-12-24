uniform sampler2D texSource[3]; // 0:main color, 1:1/4 color, 2:hdr factor.

varying mediump vec4 texCoord; // xy for main. zw for other.

#define luminance(tex) dot(tex, vec3(0.2126, 0.7152, 0.0722))

const mediump float A = 0.15; // shoulder strength.
const mediump float B = 0.50; // linear strength.
const mediump float C = 0.10; // linear angle.
const mediump float D = 0.20; // toe strength.
const mediump float E = 0.02; // toe numerator.
const mediump float F = 0.30; // toe denominator.
const mediump float W = 11.2; // linear white point.
const mediump float Gamma = 2.2;
#define Uncharteded2Tonemap(x) ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F) - E / F)
#define mkvec3(v) vec3(v, v, v)

void main()
{
  gl_FragColor = texture2D(texSource[0], texCoord.xy);
  gl_FragColor.rgb *= 2.0;

#ifdef USE_HDR_BLOOM
  gl_FragColor.rgb += texture2D(texSource[2], texCoord.zw).rgb;

  // tone mapping.
#if 0 // @ref HDRRenderingInOpenGL.pdf
  const mediump float exposure = 1.0;
  const mediump float brightMax = 1.0;
  mediump float Y = luminance(gl_FragColor.rgb);
  mediump float YD = exposure * (exposure / brightMax + 1.0) / (exposure + 1.0);
  gl_FragColor.rgb *= YD;
  gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(1.0 / Gamma, 1.0 / Gamma, 1.0 / Gamma));
#elif 0 // @ref http://filmicgames.com/archives/75
  const mediump float exposureBias = 4.0 * 2.0;
  gl_FragColor.rgb *= exposureBias;
  gl_FragColor.rgb = Uncharteded2Tonemap(gl_FragColor.rgb);

  const mediump float whiteScale = 1.0 / Uncharteded2Tonemap(W);
  gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(1.0 / Gamma, 1.0 / Gamma, 1.0 / Gamma));
#else // the optimized formula by Jim and Richard. based on color=x/(x + 1).
  gl_FragColor.rgb *= 1.0;  // Hardcoded Exposure Adjustment
  mediump vec3 x = max(gl_FragColor.rgb - 0.004, 0.0);
  gl_FragColor.rgb = (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
#endif

#endif // USE_HDR_BLOOM
}
