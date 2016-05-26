// this shader is the same as default.frag, except that the metallic in texture treated as alpha.

uniform mediump vec3 eyePos; // in world space.

uniform lowp vec3 lightColor;

uniform lowp vec4 materialColor;
uniform lowp vec2 metallicAndRoughness;
uniform lowp float dynamicRangeFactor;

uniform sampler2D texDiffuse;
uniform sampler2D texNormal;
uniform samplerCube texIBL[3];
uniform sampler2D texShadow;

varying mediump vec4 lightVectorAndDistance;
varying mediump vec3 eyeVector;
varying mediump vec3 halfVector;
varying mediump vec4 posW;
varying mediump mat3 matTBN;

varying mediump vec4 texCoord;
varying mediump vec4 posForShadow;

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
  lowp vec4 col = texture2D(texDiffuse, texCoord.xy) * materialColor;
  if (col.a == 0.0) {
	gl_FragColor = vec4(0, 0, 0, 0);
  } else {
	mediump vec3 normal;
	normal.xy = texture2D(texNormal, texCoord.xy).yw * 2.0 - 1.0;
	normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
	normal = matTBN * normal;
	mediump vec3 eyeVectorW = normalize(eyePos - posW.xyz);
	mediump vec3 refVector = reflect(eyeVectorW, normal);

	// Diffuse
	mediump vec4 diffuse = textureCube(texIBL[2], refVector);
	diffuse.rgb *= dynamicRangeFactor / diffuse.a;
	diffuse.rgb = diffuse.rgb * col.rgb * (1.0 - metallicAndRoughness.x);

	// Specular
	mediump vec3 F0;
	CalcF0(F0, col.rgb, metallicAndRoughness.x);
	mediump float dotNV = max(dot(eyeVectorW, normal), 0.0001);
	mediump vec4 specular = textureCube(texIBL[0], refVector);
	specular.rgb *= dynamicRangeFactor / max(specular.a, 1.0 / 128.0);
	specular.rgb = max(specular.rgb * FresnelSchlick(F0, dotNV), vec3(0.0, 0.0, 0.0));

	gl_FragColor.rgb = diffuse.rgb + specular.rgb;
	gl_FragColor.a = max(col.a, dot(diffuse.rgb * col.a + specular.rgb, vec3(0.3, 0.6, 0.1)));

	// Shadow
	const highp float coef = 1.0 / 256.0;
	lowp vec4 tex = texture2D(texShadow, vec2(posForShadow.x * 0.5 + 0.5, posForShadow.y * -0.5 + 0.5));
	highp float Ex = dot(tex, vec4(1.0, coef, coef * coef, coef * coef * coef));
	if (Ex < 1.0) {
	  if (posForShadow.z > Ex) {
		gl_FragColor.rgb *= exp(-80.0 * clamp(posForShadow.z - Ex, 0.0, 1.0)) * 0.6 + 0.4;
	  }
	}
  }
}
