uniform mediump vec3 eyePos; // in world space.

uniform lowp vec3 lightColor;

uniform lowp vec4 materialColor;
uniform lowp vec2 metallicAndRoughness;

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
  const highp float pi = 3.14159265358979323846264;
  const highp float maxLightRadius = 1000.0;
#if 1
  // material parameter
  lowp vec3 Idiff = vec3(0.0, 0.0, 0.0);
  mediump vec3 Ispec = vec3(0.0, 0.0, 0.0);

  lowp vec4 col = texture2D(texDiffuse, texCoord.xy);
  col.rgb *= materialColor.rgb;
  // note: the alpha component contains the metallic paramter rathar than the transparency.
  col.a = min(1.0, max(0.0, col.a + metallicAndRoughness.x));

  mediump vec4 normal = texture2D(texNormal, texCoord.xy);
  lowp vec3 normal_raw = normal.xyz;
  normal.xyz = normal.xyz * 2.0 - 1.0;
  normal.xyz = normalize(matTBN * normal.xyz);
  normal.w = min(1.0, max(0.0, normal.w + metallicAndRoughness.y));

#if 0
	mediump vec3 lightVector = lightVectorAndDistance.xyz;

    highp float distance = lightVectorAndDistance.w;
	highp float lightInfluence = distance / maxLightRadius;
	highp float fallOffSrc = clamp(1.0 - lightInfluence * lightInfluence * lightInfluence * lightInfluence, 0.0, 1.0);
    highp float attenuation = fallOffSrc * fallOffSrc / (distance * distance + 1.0);

    Idiff += attenuation * col.rgb * lightColor * max(dot(normal, lightVector), 0.0);

    // [GGX]
    // D(h) = alpha^2 / ( pi * (dot(n, h)^2 * (alpha^2 - 1) + 1)^2)
    lowp float roughness = metallicAndRoughness.y;
    highp float alpha = roughness * roughness;
    highp float alpha2 = alpha * alpha;
    highp float dotNH = dot(normal, halfVector);
    highp float x = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    highp float D = alpha2 / (pi * x * x);

    // [FGS]
    // F0 : fresnel refrectance of material. we use metallic.
    // v : eye vector.
    // h : half vector.
    // F(v, h) = F0 + (1.0 - F0) * 2^(-5.55473*dot(v, h) - 6.98316)*dot(v, h)
    lowp float metallic = metallicAndRoughness.x;
    mediump float dotVH = dot(eyeVector, halfVector);
	mediump vec3 F0;
	CalcF0(F0, col.rgb, metallic);
    mediump vec3 F = FresnelSchlick(F0, dotVH);

    // k = (roughness + 1)^2 / 8
    // G1(v) = dot(n, v) / dot(n, v) * (1 - k) + k
    // G(l, v, h) = G1(l) * G1(v)
    mediump float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    mediump float dotNL = max(dot(normal.xyz, lightVector), 0.0001);
    mediump float dotNV = max(dot(normal.xyz, eyeVector), 0.0001);
    mediump float G = G1(dotNL, k) * G1(dotNV, k);

    // f(l, v) = D(h) * F(v, h) * G(l, v, h) / (4 * dot(n, l) * dot(n, v))
	// Ž‹ü‚Æ–@ü‚ÌŠp“x‚É‚æ‚Á‚Ä‚Íf‚ªinf‚É‚È‚é‚±‚Æ‚ª‚ ‚é.
    mediump vec3 f = max((D * F * G) / (4.0 * dotNL * dotNV), 0.0);
    Ispec += lightColor * attenuation * f;

	mediump vec3 eyeVectorW = normalize(eyePos - posW.xyz);
#else
	mediump vec3 eyeVectorW = normalize(eyePos - posW.xyz);
	mediump vec3 F0;
	mediump float metal = 1.0 - col.a;
	CalcF0(F0, col.rgb, metal);
#endif

#if 1
	mediump float mipmapLevel = clamp(2.0 * normal.w, 0.0, 2.0);
	mediump vec3 refVector2 = reflect(eyeVectorW, normal.xyz);
	lowp vec4 iblColor[3];
	iblColor[0] = textureCube(texIBL[0], refVector2);
	iblColor[1] = textureCube(texIBL[1], refVector2);
	iblColor[2] = textureCube(texIBL[2], refVector2);
	// Alpha component has the strength of color. it has (255 / strength).
	// We can restore the actual color by multiply (1.0 / alpha). And we want
	// to compress the diffuse range 0-2 to 0-1, because keep the HDR
	// luminance. So, we use (0.5 / alpha).
	iblColor[0].rgb *= 0.5 / iblColor[0].a;
	iblColor[1].rgb *= 0.5 / iblColor[1].a;
	iblColor[2].rgb *= 0.5 / iblColor[2].a;

	mediump vec3 colIBL = mix(iblColor[0].rgb , iblColor[1].rgb, min(mipmapLevel, 1.0));
	colIBL = mix(colIBL, iblColor[2].rgb, max(mipmapLevel - 1.0, 0.0));
	mediump float dotNV2 = max(dot(eyeVectorW, normal.xyz), 0.0001);
	Ispec += max(colIBL * FresnelSchlick(F0, dotNV2), vec3(0.0, 0.0, 0.0));
	Idiff = iblColor[2].rgb * col.rgb * col.a;
#else
	mediump vec3 refVector2 = reflect(eyeVectorW, normal.xyz);
	Idiff += textureCube(texIBL[2], refVector2).rgb * col.rgb;
#endif

#endif
	gl_FragColor.rgb = Idiff + Ispec;
	gl_FragColor.a = max(materialColor.a, dot(Idiff * materialColor.a + Ispec, vec3(0.3, 0.6, 0.1)));
#if 0
	mediump vec4 shadowTexCoord = posForShadow;
	shadowTexCoord.xy = 0.5 * (posForShadow.xy + posForShadow.w);
	//	highp vec2 stc_div_w = shadowTexCoord.xy * (1.0 / shadowTexCoord.w);
	//	if (stc_div_w.x >= 0.01 && stc_div_w.x < 0.99 && stc_div_w.y > 0.01 && stc_div_w.y < 0.99) {
	/** ChebyshevUpperBound
	@ref http://http.developer.nvidia.com/GPUGems3/gpugems3_ch08.html
	@ref http://codeflow.org/entries/2013/feb/15/soft-shadow-mapping/
	*/
	lowp vec4 tex = texture2DProj(texShadow, shadowTexCoord);
	highp float Ex = tex.x + tex.y * (1.0 / 255.0);
	if (posForShadow.z > Ex) {
	  highp float E_x2 = (tex.z + tex.w * (1.0 / 255.0));
	  highp float variance = max(E_x2 - (Ex * Ex), 0.002);
	  highp float mD = (posForShadow.z - Ex) * 25.0;
	  highp float p = variance / (variance + mD * mD);
	  lowp float lit = min(max(p - 0.6, 0.0), 0.3) * 2.0 + 0.4;
	  gl_FragColor.rgb *= lit;
	}
	//	}
#endif
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
