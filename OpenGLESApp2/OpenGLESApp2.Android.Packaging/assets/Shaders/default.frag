uniform mediump vec3 eyePos; // in world space.

uniform lowp vec3 lightColor;

uniform lowp vec4 materialColor;
uniform lowp vec2 metallicAndRoughness;
uniform lowp float dynamicRangeFactor;

uniform sampler2D texDiffuse;
uniform sampler2D texNormal;
uniform samplerCube texIBL[3];
uniform sampler2D texShadow;

#ifdef DEBUG
uniform lowp float debug;
#endif // DEBUG

varying mediump vec4 lightVectorAndDistance;
varying mediump vec3 eyeVector;
varying mediump vec3 halfVector;
varying mediump vec4 posW;
varying mediump mat3 matTBN;

varying mediump vec4 texCoord;
varying mediump vec4 posForShadow;

// use for debugging.
varying lowp vec3 color;

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
#define CalcF0(ret, col, metallic) lowp float isMetal = step(235.0 / 255.0, metallic); ret = isMetal * col * metallic + (1.0 - isMetal) * metallic

#if 0
#define FresnelSchlick(F0, dotEH) (F0 + (1.0 - F0) * exp2((-5.55473 * dotEH - 6.98316) * dotEH))
#else
#define FresnelSchlick(F0, dotEH) (F0 + (1.0 - F0) * exp2(-8.656170 * dotEH))
#endif

void main(void)
{
  lowp vec3 col = texture2D(texDiffuse, texCoord.xy).rgb * materialColor.rgb;

  mediump vec3 normal;
  normal.xy = texture2D(texNormal, texCoord.xy).yw * 2.0 - 1.0;
  normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
  normal = normalize(matTBN * normal);
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

  gl_FragColor = vec4(diffuse.rgb + specular.rgb, materialColor.a);
  //gl_FragColor.a = max(materialColor.a, dot(diffuse.rgb * materialColor.a + specular.rgb, vec3(0.3, 0.6, 0.1)));

  // Shadow
  const highp float coef = 1.0 / 256.0;
  lowp vec4 tex = texture2D(texShadow, vec2(posForShadow.x * 0.5 + 0.5, posForShadow.y * -0.5 + 0.5));
  highp float Ex = dot(tex, vec4(1.0, coef, coef * coef, coef * coef * coef));
  if (posForShadow.z > Ex) {
	gl_FragColor.rgb *= exp(-80.0 * clamp(posForShadow.z - Ex, 0.0, 1.0)) * 0.6 + 0.4;
  }

/*
#ifdef DEBUG
    int iDebug = int(debug);
	if (iDebug == 0) {
	  mediump float nv = max(dot(eyeVectorW, vec3(matTBN[0][2], matTBN[1][2], matTBN[2][2])), 0.0001) * 0.25 + 0.25;
	  gl_FragColor.rgb = vec3(nv, nv, nv);
	} else if (iDebug == 1) {
	  gl_FragColor.rgb = vec3(dotNV2, dotNV2, dotNV2) * 0.25 + 0.25;
	} else if (iDebug == 2) {
	  gl_FragColor.rgb = normal.xyz * 0.25 + 0.25;
	} else if (iDebug == 3) {
	  gl_FragColor.rgb = vec3(matTBN[2][0], matTBN[2][1], matTBN[2][2]) * 0.25 + 0.25;
	} else if (iDebug == 4) {
	  gl_FragColor.rgb = vec3(matTBN[1][0], matTBN[1][1], matTBN[1][2]) * 0.25 + 0.25;
	} else if (iDebug == 5) {
	  gl_FragColor.rgb = vec3(matTBN[0][0], matTBN[0][1], matTBN[0][2]) * 0.25 + 0.25;
	} else if (iDebug == 6) {
	  gl_FragColor.rgb = iblColor[0].rgb;
	} else if (iDebug == 7) {
	  gl_FragColor.rgb = iblColor[1].rgb;
	} else if (iDebug == 8) {
	  gl_FragColor.rgb = iblColor[2].rgb;
	} else if (iDebug == 9) {
	  gl_FragColor.rgb = normal_raw * 0.5;
	} else if (iDebug == 10) {
	  gl_FragColor.rgb = vec3(matTBN[0][2] * 0.25 + 0.25, 0.0, 0.0);
	} else if (iDebug == 11) {
	  gl_FragColor.rgb = vec3(0.0, matTBN[1][2] * 0.25 + 0.25, 0.0);
	} else if (iDebug == 12) {
	  gl_FragColor.rgb = vec3(0.0, 0.0, matTBN[2][2] * 0.25 + 0.25);
	} else if (iDebug == 13) {
	  gl_FragColor.rgb = vec3(normal.x * 0.25 + 0.25, 0.0, 0.0);
	} else if (iDebug == 14) {
	  gl_FragColor.rgb = vec3(0.0, normal.y * 0.25 + 0.25, 0.0);
	} else if (iDebug == 15) {
	  gl_FragColor.rgb = vec3(0.0, 0.0, normal.z * 0.25 + 0.25);
	}
#endif // DEBUG
*/
}
