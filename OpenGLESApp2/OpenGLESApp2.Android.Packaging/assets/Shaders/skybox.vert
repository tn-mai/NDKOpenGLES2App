attribute highp   vec3 vPosition;

uniform mat4 matView;
uniform mat4 matProjection;

varying mediump vec3 texCoord;

void main() {
  gl_Position = matProjection * matView * vec4(vPosition, 1.0);
  texCoord = normalize(vPosition.zyx * vec3(-1.0, -1.0, 1.0));
}

