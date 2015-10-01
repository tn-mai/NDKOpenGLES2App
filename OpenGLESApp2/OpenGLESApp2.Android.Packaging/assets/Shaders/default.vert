precision highp float;

attribute highp   vec3 vPosition;
attribute mediump vec3 vNormal;
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

varying mediump vec3 pos;
varying mediump vec3 normal;
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
  pos = (m * vec4(vPosition, 1)).xyz;
  normal = normalize(mat3(m) * vNormal);
  texCoord = SCALE_TEXCOORD(vTexCoord01);
  gl_Position = (matProjection * matView * m) * vec4(vPosition, 1);
}
