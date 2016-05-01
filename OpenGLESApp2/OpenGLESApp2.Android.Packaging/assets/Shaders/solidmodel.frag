uniform mediump vec3 eyePos; // in world space.

uniform lowp vec4 materialColor;
uniform lowp vec2 metallicAndRoughness;
uniform lowp float dynamicRangeFactor;

uniform sampler2D texDiffuse;
uniform sampler2D texNormal;
uniform samplerCube texIBL[3];
uniform sampler2D texShadow;

varying mediump vec3 eyeVectorW;
//varying mediump mat3 matTBN;
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

  mediump vec4 normal = texture2D(texNormal, texCoord.xy);
  normal.xyz = normal.xzy * vec3(2.0, -2.0, -2.0) + vec3(-1.0, 1.0, 1.0);
  col.a = step(0.8, normal.w + metallicAndRoughness.x) * 0.1 + 0.9;
  normal.w = clamp((normal.w + metallicAndRoughness.y) * 2.0, 0.0, 2.0);

  mediump vec3 F0;
  mediump float metal = 1.0 - col.a;
  CalcF0(F0, col.rgb, metal);

  mediump vec3 refVector2 = reflect(eyeVectorW, normal.xyz);
  lowp vec4 iblColor[3];
//  iblColor[0] = textureCube(texIBL[0], refVector2);
//  iblColor[1] = textureCube(texIBL[1], refVector2);
  iblColor[2] = textureCube(texIBL[2], refVector2);
  // Alpha component has the strength of color. it has (255 / strength).
  // We can restore the actual color by multiply (1.0 / alpha). And we want
  // to compress the diffuse range to 0-1, because keep the HDR
  // luminance. So, we use (dynamicRangeFactor / alpha).
//  iblColor[0].rgb *= dynamicRangeFactor / max(iblColor[0].a, 1.0 / 512.0);
//  iblColor[1].rgb *= dynamicRangeFactor / iblColor[1].a;
  iblColor[2].rgb *= dynamicRangeFactor / iblColor[2].a;
  
//  mediump vec3 colIBL = mix(iblColor[0].rgb , iblColor[1].rgb, min(normal.w, 1.0));
//  colIBL = mix(colIBL, iblColor[2].rgb, max(normal.w - 1.0, 0.0));
  mediump float dotNV2 = dot(eyeVectorW, normal.xyz);
  mediump vec3 Ispec = iblColor[2].rgb * FresnelSchlick(F0, dotNV2);
  lowp vec3 Idiff = iblColor[2].rgb * col.rgb * col.a;
  
  gl_FragColor = vec4(Idiff + Ispec, 1.0);
//  gl_FragColor = vec4(normal.xyz * 0.125 + 0.125, 1.0);
//  gl_FragColor.a = max(materialColor.a, dot(Idiff * materialColor.a + Ispec, vec3(0.3, 0.6, 0.1)));
  
  const mediump float coef = 1.0 / 256.0;
  mediump vec4 tex = texture2D(texShadow, posForShadow.xy);
  highp float Ex = dot(tex.xyz, vec3(1.0, coef, coef * coef));
  if (Ex < 1.0) {
    if (posForShadow.z > Ex) {
  	gl_FragColor.rgb *= exp(-80.0 * clamp(posForShadow.z - Ex, 0.0, 1.0)) * 0.6 + 0.4;
    }
  }
}
