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
  lowp vec4 col = texture2D(texDiffuse, texCoord.xy);
  col.rgb *= materialColor.rgb;

//  if (col.a == 0.0) {
//	gl_FragColor = vec4(0, 0, 0, 0);
//  } else {

	// NOTE: This shader uses the object space normal mapping, so the normal is in the object space.
	//       In general, the object space normal mapping is used the pre-transformed light position
	//       in the object spece. But this shader is the image based light source in the world space.
	//       Of course, it cannot pre-transform to the object space.
	//       Therefore, the object space normal mapping cannot increase efficiency of the shader
	//       that contrary to our expectations :(
	mediump vec3 normal = texture2D(texNormal, texCoord.xy).xyz * 2.0 - 1.0;
	mediump vec3 refVector = matTBN * reflect(eyeVectorW, normal.xyz);

	// Diffuse
	mediump vec4 irradiance = textureCube(texIBL[2], refVector);
	irradiance.rgb *= dynamicRangeFactor / irradiance.a;
	gl_FragColor.rgb = irradiance.rgb * col.rgb * (1.0 - metallicAndRoughness.x);

	// Specular
	mediump vec3 F0;
	CalcF0(F0, col.rgb, metallicAndRoughness.x);
	mediump float dotNV = dot(eyeVectorW, normal.xyz);
	mediump vec4 specular = textureCube(texIBL[0], refVector);
	specular.rgb *= dynamicRangeFactor / specular.a;
	gl_FragColor.rgb += specular.rgb * FresnelSchlick(F0, dotNV);

	gl_FragColor.a = materialColor.a;
//	lowp float alpha = col.a * materialColor.a;
//	gl_FragColor.a = max(alpha, dot(Idiff * alpha + Ispec, vec3(0.3, 0.6, 0.1)));

	// Shadow
	const mediump float coef = 1.0 / 256.0;
	mediump vec4 tex = texture2D(texShadow, posForShadow.xy);
	highp float Ex = dot(tex, vec4(1.0, coef, coef * coef, coef * coef * coef));
	if (Ex < 1.0) {
	  if (posForShadow.z > Ex) {
		gl_FragColor.rgb *= exp(-80.0 * clamp(posForShadow.z - Ex, 0.0, 1.0)) * 0.6 + 0.4;
	  }
	}

//  }
}
