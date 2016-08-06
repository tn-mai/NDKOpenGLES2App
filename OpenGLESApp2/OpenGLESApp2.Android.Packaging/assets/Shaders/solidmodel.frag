uniform mediump vec3 eyePos; // in world space.

uniform lowp vec4 materialColor;
uniform lowp vec2 metallicAndRoughness;
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

void main(void)
{
  lowp vec3 col = texture2D(texDiffuse, texCoord.xy).rgb * materialColor.rgb;

  // NOTE: This shader uses the object space normal mapping, so the normal is in the object space.
  //       In general, the object space normal mapping is used the pre-transformed light position
  //       in the object spece. But this shader is the image based light source in the world space.
  //       Of course, it cannot pre-transform to the object space.
  //       Therefore, the object space normal mapping cannot increase efficiency of the shader
  //       that contrary to our expectations :(
  mediump vec3 normal = texture2D(texNormal, texCoord.xy).xyz * 2.0 - 1.0;
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
  if (posForShadow.z > Ex) {
	// puseudo GL_CLAMP_TO_BORDER effect.
	const lowp vec2 dims = vec2(SHADOWMAP_MAIN_WIDTH, SHADOWMAP_MAIN_HEIGHT);
	lowp vec2 f = clamp(-abs(dims * (posForShadow.xy - 0.5)) + (dims + 1.0) * 0.5, 0.0, 1.0);
	gl_FragColor.rgb *= 1.0 - 0.4 * f.x * f.y;// exp(-80.0 * clamp(posForShadow.z - Ex, 0.0, 1.0)) * 0.6 + 0.4;
  }
}
