precision highp float;

attribute highp   vec3 vPosition;
attribute mediump vec3 vNormal;
attribute lowp    vec4 vWeight;
attribute mediump vec4 vBoneID;

uniform highp mat4 matLightForShadow;
uniform highp vec4 boneMatrices[32 * 3];

varying highp float depth;

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

	mediump vec3 normalW = normalize(mat3(m) * vNormal);
	const mediump vec3 shadowLightDir = normalize(vec3(-0.2, 1.0, -0.2));
	mediump float bias = 0.015 * clamp(1.0 - dot(normalW, shadowLightDir), 0.3, 1.0);

	gl_Position = matLightForShadow * m * vec4(vPosition, 1);
	depth = gl_Position.z * 0.5 + 0.5 + bias;
}
