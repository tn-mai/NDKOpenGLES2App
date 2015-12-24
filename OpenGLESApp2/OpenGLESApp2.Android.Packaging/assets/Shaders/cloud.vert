precision highp float;

attribute highp   vec3 vPosition;
attribute mediump vec3 vNormal;
attribute mediump vec4 vTangent;
attribute mediump vec4 vTexCoord01;
attribute lowp    vec4 vWeight;
attribute mediump vec4 vBoneID;

uniform mat4 matView;
uniform mat4 matProjection;
uniform mat4 matLightForShadow;

/* The arrey of 3x4 matrix.
* Any matrix is the Model-View matrix.
*/
uniform vec4 boneMatrices[32*3];

uniform mediump vec3 lightPos; // in view space.
uniform mediump vec3 eyePos; // in world space.

varying mediump vec4 lightVectorAndDistance;
varying mediump vec3 eyeVector;
varying mediump vec3 halfVector;
varying mediump vec4 posW;
varying mediump vec3 normalW;
varying mediump vec3 tangentW;
varying mediump vec3 binormalW;

varying mediump vec4 texCoord;
varying mediump vec4 posForShadow;

varying lowp vec3 color;

void main()
{
  mat4 m;
  m[0] = vec4(boneMatrices[0].xyz, 0);
  m[1] = vec4(boneMatrices[1].xyz, 0);
  m[2] = vec4(boneMatrices[2].xyz, 0);
  m[3] = vec4(boneMatrices[0].w, boneMatrices[1].w, boneMatrices[2].w, 1);

  posForShadow = matLightForShadow * m * vec4(vPosition, 1);
  posForShadow.z = posForShadow.z * 0.5 + 0.5;
  //posForShadow.xy = 0.5 * (posForShadow.xy + posForShadow.w);

  // Strictly, we may use 'transpose(inverse(M))' matrix.
  // But in most case, the matrix is orthonormal(when not include the scale factor).
  // Therefore, we can be used it to transform the normal.
  // note: In the orthonormal matrix, inverse(M) equals transpose(M), thus transpose(inverse(M)) equals transpose(transpose(M)). That result equals M.
  mat3 m3 = mat3(m);
  normalW = normalize(m3 * vNormal);
  tangentW = normalize(m3 * vTangent.xyz);
  binormalW = cross(normalW, tangentW) *vTangent.w;

  posW = m * vec4(vPosition, 1);

  mediump vec3 lightVec = lightPos - posW.xyz;
  lightVectorAndDistance.x = dot(lightVec, tangentW);
  lightVectorAndDistance.y = dot(lightVec, binormalW);
  lightVectorAndDistance.z = dot(lightVec, normalW);
  lightVectorAndDistance.w = length(lightVectorAndDistance.xyz);
  lightVectorAndDistance.xyz = normalize(lightVectorAndDistance.xyz);

  vec3 eyeVec = eyePos - posW.xyz;
  eyeVector.x = dot(eyeVec, tangentW);
  eyeVector.y = dot(eyeVec, binormalW);
  eyeVector.z = dot(eyeVec, normalW);
  eyeVector = normalize(eyeVector);

  halfVector = normalize(eyeVector + lightVectorAndDistance.xyz);

  texCoord = SCALE_TEXCOORD(vTexCoord01);
  gl_Position = (matProjection * matView * m) * vec4(vPosition, 1);
}
