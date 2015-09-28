#version 100

uniform mediump vec3 eyePos;

uniform lowp vec3 lightColor;
uniform mediump vec3 lightPos;

uniform lowp vec4 materialColor;
uniform lowp vec2 metallicAndRoughness;

uniform sampler2D texDiffuse;
uniform sampler2D texNormal;
uniform sampler2D texMetalRoughness;
uniform samplerCube texIBL[3];
uniform sampler2D texShadow;

varying mediump vec3 pos;
varying mediump vec3 normal;
varying mediump vec4 texCoord;
varying mediump vec4 posForShadow;

// use for debugging.
varying lowp vec3 color;

mediump float G1(mediump float dottedFactor, mediump float k)
{
  return dottedFactor / (dottedFactor * (1.0 - k) + k);
}

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
mediump vec3 CalcF0(lowp vec3 col, lowp float metallic)
{
  const lowp float dielectricRange = 235.0 / 255.0;
//  const lowp float metalRange = 20.0 / 255.0;
//  mediump float F0_Dielectric = clamp(metallic * (1.0 / dielectricRange), 0.0, 1.0) * 0.067 * (1.0 - step(dielectricRange, metallic));
//  mediump float F0_Metal = clamp((metallic - dielectricRange) * (1.0 / metalRange), 0.0, 1.0) * 0.3 + step(dielectricRange, metallic) * 0.7;
  lowp float isMetal = step(dielectricRange, metallic);
  return isMetal * col * metallic + (1.0 - isMetal) * metallic;
}

mediump vec3 FresnelSchlick(mediump vec3 F0, mediump float dotEH)
{
  return F0 + (1.0 - F0) * exp2((-5.55473 * dotEH - 6.98316) * dotEH);
}

highp float ldexp(lowp vec2 v)
{
	return v.x * exp2(v.y * (-255.0));
}

/**
  @ref http://http.developer.nvidia.com/GPUGems3/gpugems3_ch08.html
  @ref http://codeflow.org/entries/2013/feb/15/soft-shadow-mapping/
*/
lowp float ChebyshevUpperBound(mediump vec4 tex, highp float curDepth)
{
  highp float Ex = tex.x + tex.y * (1.0 / 255.0);
//  curDepth -= 0.005;
  if (curDepth <= Ex) {
	return 1.0;
  }
  highp float E_x2 = (tex.z + tex.w * (1.0 / 255.0));
  //highp float E_x2 = ldexp(tex.zw);
  highp float variance = max(E_x2 - (Ex * Ex), 0.002);
  highp float mD = (curDepth - Ex) * 200.0;
  highp float p = variance / (variance + mD * mD);
  return clamp(2.0 * p - p * p, 0.5, 1.0);
}

void main(void)
{
  const highp float pi = 3.14159265358979323846264;
  const highp float maxLightRadius = 1000.0;
#if 1
  // material parameter
  mediump vec3 Idiff = vec3(0.0, 0.0, 0.0);
  mediump vec3 Ispec = vec3(0.0, 0.0, 0.0);
  lowp vec4 col = texture2D(texDiffuse, texCoord.xy) * materialColor;

    mediump vec3 eyeVector = normalize(eyePos - pos);
	mediump vec3 vertexToLight = lightPos - pos;
	mediump vec3 lightVector = normalize(vertexToLight);

    highp float distance = length(vertexToLight);
	highp float lightInfluence = distance / maxLightRadius;
	highp float fallOffSrc = clamp(1.0 - lightInfluence * lightInfluence * lightInfluence * lightInfluence, 0.0, 1.0);
    highp float attenuation = fallOffSrc * fallOffSrc / (distance * distance + 1.0);

    Idiff += attenuation * col.rgb * lightColor * max(dot(normal, lightVector), 0.0);

    // [GGX]
    // D(h) = alpha^2 / ( pi * (dot(n, h)^2 * (alpha^2 - 1) + 1)^2)
    lowp float roughness = metallicAndRoughness.y;
    mediump vec3 halfVector = normalize(eyeVector + lightVector);
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
    mediump vec3 F0 = CalcF0(col.rgb, metallic);
    mediump vec3 F = FresnelSchlick(F0, dotVH);

    // k = (roughness + 1)^2 / 8
    // G1(v) = dot(n, v) / dot(n, v) * (1 - k) + k
    // G(l, v, h) = G1(l) * G1(v)
    mediump float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    mediump float dotNL = max(dot(normal, lightVector), 0.0001);
    mediump float dotNV = max(dot(normal, eyeVector), 0.0001);
    mediump float G = G1(dotNL, k) * G1(dotNV, k);

    // f(l, v) = D(h) * F(v, h) * G(l, v, h) / (4 * dot(n, l) * dot(n, v))
	// Ž‹ü‚Æ–@ü‚ÌŠp“x‚É‚æ‚Á‚Ä‚Íf‚ªinf‚É‚È‚é‚±‚Æ‚ª‚ ‚é.
    mediump vec3 f = max((D * F * G) / (4.0 * dotNL * dotNV), 0.0);
    Ispec += lightColor * attenuation * f;

#if 1
	mediump float mipmapLevel = clamp(2.0 * roughness, 0.0, 2.0);
	mediump vec3 refVector2 = reflect(eyeVector, normal);
	lowp vec3 iblColor[3];
	iblColor[0] = textureCube(texIBL[0], refVector2).rgb;
	iblColor[1] = textureCube(texIBL[1], refVector2).rgb;
	iblColor[2] = textureCube(texIBL[2], refVector2).rgb;
	mediump vec3 colIBL = mix(iblColor[0], iblColor[1], min(mipmapLevel, 1.0));
	colIBL = mix(colIBL, iblColor[2], max(mipmapLevel - 1.0, 0.0));
	mediump vec3 hdrFactor = max(vec3(0.0, 0.0, 0.0), colIBL - 240.0 / 255.0);
	colIBL += hdrFactor * 31.0;
	Ispec += colIBL * FresnelSchlick(F0, dotNV);
	Idiff += iblColor[2] * col.rgb * (1.0 - metallic);
#else
	mediump vec3 refVector2 = reflect(eyeVector, normal);
	Idiff += textureCube(texIBLDiffuse, refVector2).rgb * col.rgb;
#endif

#endif

	gl_FragColor = vec4(Idiff + Ispec, col.a);

#if 1
	const highp float near = 10.0;
	const highp float far = 200.0;
	const highp float linerDepth = 1.0 / (far - near);
	highp float curDepth = min(length(posForShadow.xyz) * linerDepth, 1.0);
	mediump vec4 shadowTexCoord = posForShadow;
	shadowTexCoord.xy = 0.5 * (posForShadow.xy + posForShadow.w);
	highp vec2 stc_div_w = shadowTexCoord.xy * (1.0 / shadowTexCoord.w);
	if (stc_div_w.x >= 0.01 && stc_div_w.x < 0.99 && stc_div_w.y > 0.01 && stc_div_w.y < 0.99) {
	  lowp float lit = ChebyshevUpperBound(texture2DProj(texShadow, shadowTexCoord), curDepth);
	  gl_FragColor.rgb *= lit;
//	} else {
//	  gl_FragColor = vec4(vec3(1.0, 1.0, 1.0) - (Idiff + Ispec), col.a);
	}
#else
	lowp float lit = 1.0;
#endif
  //gl_FragColor = vec4(step(shadowTexCoord.x, posForShadow.w) * step(0.0, shadowTexCoord.x), step(shadowTexCoord.y, posForShadow.w) * step(0.0, shadowTexCoord.y), shadowFactor.x, 1.0);
  //gl_FragColor = vec4(color, 1.0);
}
