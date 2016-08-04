uniform mediump vec3 eyePos; // in world space.

uniform lowp vec4 materialColor;
uniform lowp vec3 metallicAndRoughness;
uniform lowp float dynamicRangeFactor;

uniform sampler2D texDiffuse;
uniform sampler2D texNormal;
uniform samplerCube texIBL[3];
uniform sampler2D texShadow;

varying mediump mat3 matTBN;
varying mediump vec3 eyeVectorW;
varying mediump vec4 texCoord;
varying mediump vec3 posForShadow;

#define G1(dottedFactor, k) \
  (dottedFactor / (dottedFactor * (1.0 - k) + k))

// [FGS]
// F0 : fresnel refrectance of material.
//      dielectric = 0.017-0.067(the average to 0.04)
//      metal = 0.7-1.0
// v  : eye vector.
// h  : half vector. for IBL, this is equal to the normal.
// F(v, h) = F0 + (1.0 - F0) * 2^(-5.55473*dot(v, h) - 6.98316)*dot(v, h)
// reference:
//   http://d.hatena.ne.jp/hanecci/20130727/p2
//   https://seblagarde.wordpress.com/2011/08/17/hello-world/
#define CalcF0(ret, col, metallic) \
  lowp float isMetal = step(235.0 / 255.0, metallic); \
  ret = isMetal * col * metallic + (1.0 - isMetal) * metallic;

#if 0
#define FresnelSchlick(F0, dotEH) \
  (F0 + (1.0 - F0) * exp2((-5.55473 * dotEH - 6.98316) * dotEH))
#else
#define FresnelSchlick(F0, dotEH) \
  (F0 + (1.0 - F0) * exp2(-8.656170 * dotEH))
#endif

/**
* Generate a random value.
* The canonical "fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453)" didn't work on Mali-400, so replaced sin() w/ approximation.
* @ref https://gist.github.com/johansten/3633917
*/
mediump float random(mediump vec2 st) {
#if 0
  mediump float a = fract(dot(st, vec2(2.067390879775102, 12.451168662908249))) - 0.5;
  mediump float s = a * (6.182785114200511 + a*a * (-38.026512460676566 + a*a * 53.392573080032137));
  return fract(s * 43758.5453);
#else
  return fract(sin(dot(st.xy ,vec2(12.9898,78.233))) * 43758.5453);
#endif
}

mediump vec3 vec_noise(mediump vec2 st) {
  mediump vec2 i = floor(st);
  mediump vec2 f = fract(st);

  mediump vec4 abcd = vec4(
	random(i),
	random(i + vec2(1.0, 0.0)),
	random(i + vec2(0.0, 1.0)),
	random(i + vec2(1.0, 1.0))
  );

  mediump vec2 fa = abs(f * 2.0 - 1.0);
  mediump vec4 tmp = mix(abcd.ywzw - abcd.xzxy, vec4(0.0, 0.0, 0.0, 0.0), fa.xxyy);
  mediump vec2 tmp2 = mix(tmp.xz, tmp.yw, f.yx);
  return normalize(cross(vec3(0.0, tmp2.y, 1.0), vec3(1.0, tmp2.x, 0.0)));
}

void main(void)
{
  lowp vec3 col = texture2D(texDiffuse, texCoord.xy).rgb * materialColor.rgb;

  mediump vec3 noiseVec = vec_noise(texCoord.xy * 1000.0 + metallicAndRoughness.z);
  noiseVec += vec_noise(texCoord.xy * 381.0 + vec2(-1.0, 0.9) * metallicAndRoughness.z);
  noiseVec *= 0.5;

  // NOTE: This shader uses the object space normal mapping, so the normal is in the object space.
  //       In general, the object space normal mapping is used the pre-transformed light position
  //       in the object spece. But this shader is the image based light source in the world space.
  //       Of course, it cannot pre-transform to the object space.
  //       Therefore, the object space normal mapping cannot increase efficiency of the shader
  //       that contrary to our expectations :(
  mediump vec3 normal = mix(texture2D(texNormal, texCoord.xy).xyz * 2.0 - 1.0, noiseVec, dot(col, vec3(-2.0, 1.0, 1.0) * 8.0));
  mediump vec3 refVector = normalize(matTBN * reflect(eyeVectorW, normal.xyz));

  // Diffuse
  mediump vec4 irradiance = textureCube(texIBL[2], refVector);
  irradiance.rgb *= dynamicRangeFactor / irradiance.a;
  gl_FragColor = vec4(irradiance.rgb * col * (1.0 - metallicAndRoughness.x), 1.0);

  // Specular
  mediump vec3 F0;
  CalcF0(F0, col, metallicAndRoughness.x);
  mediump float dotNV = dot(eyeVectorW, normal);
  mediump vec4 specular = textureCube(texIBL[0], refVector);
  specular.rgb *= dynamicRangeFactor / specular.a;
  gl_FragColor.rgb += specular.rgb * FresnelSchlick(F0, dotNV);

  // Shadow
  const mediump float coef = 1.0 / 256.0;
  mediump vec4 tex = texture2D(texShadow, posForShadow.xy);
  highp float Ex = dot(tex, vec4(1.0, coef, coef * coef, coef * coef * coef));
  if (Ex < 1.0) {
	if (posForShadow.z > Ex) {
	  gl_FragColor.rgb *= 0.6;// exp(-80.0 * clamp(posForShadow.z - Ex, 0.0, 1.0)) * 0.6 + 0.4;
	}
  }
}
