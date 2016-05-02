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
uniform vec4 boneMatrices[3];

uniform mediump vec3 eyePos; // in object space.

varying mediump mat3 matTBN;
varying mediump vec3 eyeVectorW;
varying mediump vec4 texCoord;
varying mediump vec3 posForShadow;

void main()
{
  mat4 m;
  m[0] = vec4(boneMatrices[0].xyz, 0);
  m[1] = vec4(boneMatrices[1].xyz, 0);
  m[2] = vec4(boneMatrices[2].xyz, 0);
  m[3] = vec4(boneMatrices[0].w, boneMatrices[1].w, boneMatrices[2].w, 1);

  matTBN = mat3(m);

  posForShadow = (matLightForShadow * m * vec4(vPosition, 1)).xyz;
  posForShadow = posForShadow * vec3(0.5, -0.5, 0.5) + vec3(0.5, 0.5, 0.5);

  eyeVectorW = normalize(eyePos - vPosition);

  texCoord = SCALE_TEXCOORD(vTexCoord01);
  gl_Position = (matProjection * matView * m) * vec4(vPosition, 1);
}
