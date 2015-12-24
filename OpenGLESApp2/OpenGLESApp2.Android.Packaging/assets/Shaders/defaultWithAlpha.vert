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
varying mediump mat3 matTBN;

varying mediump vec4 texCoord;
varying mediump vec4 posForShadow;

varying lowp vec3 color;

void main()
{
  vec4 bid = vBoneID * 3.0;
  int b0 = int(bid.x);
  int b1 = int(bid.y);
  int b2 = int(bid.z);
  int b3 = int(bid.w);
  vec4 w = SCALE_BONE_WEIGHT(vWeight); // weight must be normalized, because it has 0-255.
  vec4 v0 = boneMatrices[b0 + 0] * w.x + boneMatrices[b1 + 0] * w.y + boneMatrices[b2 + 0] * w.z + boneMatrices[b3 + 0] * w.w;
  vec4 v1 = boneMatrices[b0 + 1] * w.x + boneMatrices[b1 + 1] * w.y + boneMatrices[b2 + 1] * w.z + boneMatrices[b3 + 1] * w.w;
  vec4 v2 = boneMatrices[b0 + 2] * w.x + boneMatrices[b1 + 2] * w.y + boneMatrices[b2 + 2] * w.z + boneMatrices[b3 + 2] * w.w;
  mat4 m;
  m[0] = vec4(v0.xyz, 0);
  m[1] = vec4(v1.xyz, 0);
  m[2] = vec4(v2.xyz, 0);
  m[3] = vec4(v0.w, v1.w, v2.w, 1);

  posForShadow = matLightForShadow * m * vec4(vPosition, 1);
  posForShadow.z = posForShadow.z * 0.5 + 0.5;
  //posForShadow.xy = 0.5 * (posForShadow.xy + posForShadow.w);

  // Strictly, we may use 'transpose(inverse(M))' matrix.
  // But in most case, the matrix is orthonormal(when not include the scale factor).
  // Therefore, we can be used it to transform the normal.
  // note: In the orthonormal matrix, inverse(M) equals transpose(M), thus transpose(inverse(M)) equals transpose(transpose(M)). That result equals M.
  //matTBN = mat3(m) * mat3(vTangent.xyz, normalize(cross(vNormal, vTangent.xyz)) * vTangent.w, vNormal);
  mediump mat3 m3 = mat3(m);
  mediump vec3 normalW = normalize(m3 * vNormal);
  mediump vec3 tangentW = normalize(m3 * vTangent.xyz);
  mediump vec3 binormalW = normalize(cross(normalW, tangentW)) * vTangent.w;
  matTBN = mat3(
	tangentW.x, binormalW.x, normalW.x,
	tangentW.y, binormalW.y, normalW.y,
	tangentW.z, binormalW.z, normalW.z
  );

  posW = m * vec4(vPosition, 1);

  mediump vec3 lightVec = lightPos - posW.xyz;
  lightVectorAndDistance.xyz = matTBN * lightVec;
  lightVectorAndDistance.w = length(lightVectorAndDistance.xyz);
  lightVectorAndDistance.xyz = normalize(lightVectorAndDistance.xyz);

  mediump vec3 eyeVec = eyePos - posW.xyz;
  eyeVector = matTBN * eyeVec;
  eyeVector = normalize(eyeVector);

  halfVector = normalize(eyeVector + lightVectorAndDistance.xyz);

  texCoord = SCALE_TEXCOORD(vTexCoord01);
  gl_Position = (matProjection * matView * m) * vec4(vPosition, 1);
  //color = vTangent * 0.5 + 0.5;
}
