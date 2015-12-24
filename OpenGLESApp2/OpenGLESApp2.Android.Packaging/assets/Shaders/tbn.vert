precision mediump float;

attribute mediump vec3 vPosition;
attribute lowp    vec4 vWeight;
attribute mediump vec4 vBoneID;
attribute lowp    vec4 vColor;

uniform mat4 matView;
uniform mat4 matProjection;

/* The arrey of 3x4 matrix.
* Any matrix is the Model-View matrix.
*/
uniform vec4 boneMatrices[32 * 3];

varying lowp vec4 color;

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

  gl_Position = (matProjection * matView * m) * vec4(vPosition, 1);
  color = vColor;
}
