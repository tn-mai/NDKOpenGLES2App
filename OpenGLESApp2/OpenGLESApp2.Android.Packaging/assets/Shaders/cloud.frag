uniform mediump vec3 eyePos; // in world space.

uniform lowp vec3 cloudColor; // the edge color of cloud.
uniform lowp vec4 materialColor; // the main color of cloud.
uniform lowp float dynamicRangeFactor;

uniform sampler2D texDiffuse;
uniform sampler2D texShadow;

varying mediump vec4 texCoord;
varying mediump vec4 posForShadow;

void main(void)
{
  lowp float alpha = texture2D(texDiffuse, texCoord.xy).g;
  gl_FragColor = vec4(mix(cloudColor, materialColor.rgb, alpha * alpha), alpha * materialColor.a);

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
