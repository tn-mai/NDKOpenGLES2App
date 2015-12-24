uniform mediump vec3 eyePos; // in world space.

uniform lowp vec4 materialColor;

uniform sampler2D texDiffuse;
uniform samplerCube texIBL[3];
uniform sampler2D texShadow;

varying mediump vec4 lightVectorAndDistance;
varying mediump vec3 eyeVector;
varying mediump vec3 halfVector;
varying mediump vec4 posW;
varying mediump vec3 normalW;
varying mediump vec3 tangentW;
varying mediump vec3 binormalW;

varying mediump vec4 texCoord;
varying mediump vec4 posForShadow;

// use for debugging.
varying lowp vec3 color;

void main(void)
{
  lowp vec4 col = texture2D(texDiffuse, texCoord.xy);

  mediump vec3 eyeVectorW = normalize(eyePos - posW.xyz);
  mediump vec3 refVector2 = reflect(eyeVectorW, normalW);
  lowp vec4 iblColor = textureCube(texIBL[2], refVector2);
  // Alpha component has the strength of color. it has (255 / strength).
  // We can restore the actual color by multiply (1.0 / alpha). And we want
  // to compress the diffuse range 0-2 to 0-1, because keep the HDR
  // luminance. So, we use (0.5 / alpha).
  iblColor.rgb *= 0.5 / iblColor.a;
  
  gl_FragColor = vec4(mix(iblColor.rgb, materialColor.rgb, col.rgb * col.r), col.r * materialColor.a);

#if 0
  mediump vec4 shadowTexCoord = posForShadow;
  shadowTexCoord.xy = 0.5 * (posForShadow.xy + posForShadow.w);
//  highp vec2 stc_div_w = shadowTexCoord.xy * (1.0 / shadowTexCoord.w);
//  if (stc_div_w.x >= 0.01 && stc_div_w.x < 0.99 && stc_div_w.y > 0.01 && stc_div_w.y < 0.99) {
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
//  }
#endif
}
