#include "Mesh.h"
#include <algorithm>

namespace {

  uint32_t GetValue(const uint8_t* pBuf, size_t size) {
	const uint8_t* p = reinterpret_cast<const uint8_t*>(pBuf);
	uint32_t result = 0;
	uint32_t scale = 1;
	for (size_t i = 0; i < size; ++i) {
	  result += p[i] * scale;
	  scale *= 0x100;
	}
	return result;
  }
  float GetFloat(const uint8_t*& pBuf) {
	const uint32_t tmp = GetValue(pBuf, 4);
	pBuf += 4;
	return *reinterpret_cast<const float*>(&tmp);
  }

} // unnamed namespace

/** original mesh file format.

  char[3]           "MSH"
  uint8_t           mesh count.
  uint32_t          vbo offset by the top of file(32bit alignment).
  uint32_t          vbo byte size(32bit alignment).
  uint32_t          ibo byte size(32bit alignment).
  [
    uint32_t        ibo offset.
    uint32_t        ibo size.
    uint8_t         mesh name length.
    char[length]    mesh name.
    padding         (4 - (length + 1) % 4) % 4 byte.
  ] x (mesh count)
  vbo               vbo data.
  ibo               ibo data.

  padding           (4 - (ibo byte size % 4) % 4 byte.

  uint16_t          bone count.
  uint16_t          animation count.

  [
    RotTrans        rotation and translation for the bind pose.
	int32_t         parent bone index.
  ] x (bone count)

  [
    uint8_t         animation name length.
    char[24]        animation name.
    bool            loop flag
    uint16_t        key frame count.
    float           total time.
    [
      float         time.
      [
        RotTrans    rotation and translation.
      ] x (bone count)
    ] x (key frame count)
  ] x (animation count)
*/
ImportMeshResult ImportMesh(const RawBuffer& data, GLuint& vbo, GLintptr& vboEnd, GLuint& ibo, GLintptr& iboEnd)
{
  ImportMeshResult  result;
  const uint8_t* p = &data[0];
  const uint8_t* pEnd = p + data.size();
  if (p[0] != 'M' || p[1] != 'S' || p[2] != 'H') {
	return result;
  }
  p += 3;
  const int count = *p;
  p += 1;
  /*const uint32_t vboOffset = GetValue(p, 4);*/ p += 4;
  const uint32_t vboByteSize = GetValue(p, 4); p += 4;
  const uint32_t iboByteSize = GetValue(p, 4); p += 4;
  if (p >= pEnd) {
	return ImportMeshResult();
  }

  GLuint iboBaseOffset = iboEnd;
  result.meshes.reserve(count);
  for (int i = 0; i < count; ++i) {
	Mesh m;
	m.iboOffset = iboBaseOffset; p += 4;
	m.iboSize = GetValue(p, 4); p += 4;
	const uint32_t nameLength = *p++;
	m.id.assign(p, p + nameLength); p += nameLength;
	result.meshes.push_back(m);
	iboBaseOffset += m.iboSize * sizeof(GLushort);
	p += (4 - (nameLength + 1) % 4) % 4;
	if (p >= pEnd) {
	  return ImportMeshResult();
	}
  }

  glBufferSubData(GL_ARRAY_BUFFER, vboEnd, vboByteSize, p);
  p += vboByteSize;
  if (p >= pEnd) {
	return ImportMeshResult();
  }

  std::vector<GLushort>  indices;
  indices.reserve(iboByteSize / sizeof(GLushort));
  const GLushort offset = vboEnd / sizeof(Vertex);
  for (uint32_t i = 0; i < iboByteSize; i += sizeof(GLushort)) {
	indices.push_back(*reinterpret_cast<const GLushort*>(p) + offset);
	p += sizeof(GLushort);
	if (p >= pEnd) {
	  return ImportMeshResult();
	}
  }
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, iboEnd, iboByteSize, &indices[0]);

  vboEnd += vboByteSize;
  iboEnd += iboByteSize;

  p += (4 - (reinterpret_cast<intptr_t>(p) % 4)) % 4;
  if (p >= pEnd) {
	return result;
  }

  const uint32_t boneCount = GetValue(p, 2); p += 2;
  const uint32_t animationCount = GetValue(p, 2); p += 2;

  if (boneCount) {
	JointList joints;
	joints.resize(boneCount);
	std::vector<std::vector<int>> parentIndexList;
	parentIndexList.resize(boneCount);
	for (int i = 0; i < boneCount; ++i) {
	  if (p >= pEnd) {
		return ImportMeshResult();
	  }
	  Joint& e = joints[i];
	  e.invBindPose.rot.x = GetFloat(p);
	  e.invBindPose.rot.y = GetFloat(p);
	  e.invBindPose.rot.z = GetFloat(p);
	  e.invBindPose.rot.w = GetFloat(p);
	  e.invBindPose.rot.Normalize();
	  e.invBindPose.trans.x = GetFloat(p);
	  e.invBindPose.trans.y = GetFloat(p);
	  e.invBindPose.trans.z = GetFloat(p);
#if 0
	  const Matrix4x3 m43 = ToMatrix(e.invBindPose.rot);
	  Matrix4x4 m44;
	  m44.SetVector(0, m43.GetVector(0));
	  m44.SetVector(1, m43.GetVector(1));
	  m44.SetVector(2, m43.GetVector(2));
	  m44.SetVector(3, e.invBindPose.trans);
	  m44.Inverse();
	  Vector3F scale;
	  m44.Decompose(&e.initialPose.rot, &scale, &e.initialPose.trans);
#else
	  e.initialPose.rot = e.invBindPose.rot.Inverse();
	  e.initialPose.trans = e.initialPose.rot.Apply(-e.invBindPose.trans);
#endif
	  e.offChild = 0;
	  e.offSibling = 0;
	  const uint32_t parentIndex = GetValue(p, 4); p += 4;
	  if (parentIndex != 0xffffffff) {
		parentIndexList[parentIndex].push_back(i);
	  }
	}
	for (int i = 0; i < boneCount; ++i) {
	  const auto& e = parentIndexList[i];
	  if (!e.empty()) {
		int current = e[0];
		joints[i].offChild = current - i;
		Joint* pJoint = &joints[current];
		for (auto itr = e.begin() + 1; itr != e.end(); ++itr) {
		  const int sibling = *itr;
		  pJoint->offSibling = sibling - current;
		  pJoint = &joints[sibling];
		  current = sibling;
		}
	  }
	}
	for (auto& e : result.meshes) {
	  e.jointList = joints;
	}
  }

  if (animationCount) {
	result.animations.reserve(animationCount);
	for (int i = 0; i < animationCount; ++i) {
	  Animation anm;
	  const uint32_t nameLength = GetValue(p++, 1);
	  char name[24];
	  for (int i = 0; i < 24; ++i) {
		name[i] = static_cast<char>(GetValue(p++, 1));
	  }
	  anm.id.assign(name, name + nameLength);
	  anm.data.resize(boneCount);
	  for (int bone = 0; bone < boneCount; ++bone) {
		anm.data[bone].first = bone;
	  }
	  anm.loopFlag = static_cast<bool>(GetValue(p++, 1));
	  const uint32_t keyframeCount = GetValue(p, 2); p += 2;
	  anm.totalTime = GetFloat(p);
	  for (int keyframe = 0; keyframe < keyframeCount; ++keyframe) {
		const float time = GetFloat(p);
		for (int bone = 0; bone < boneCount; ++bone) {
		  if (p >= pEnd) {
			return ImportMeshResult();
		  }
		  Animation::Element elem;
		  elem.time = time;
		  elem.pose.rot.x = GetFloat(p);
		  elem.pose.rot.y = GetFloat(p);
		  elem.pose.rot.z = GetFloat(p);
		  elem.pose.rot.w = GetFloat(p);
		  elem.pose.rot.Normalize();
		  elem.pose.trans.x = GetFloat(p);
		  elem.pose.trans.y = GetFloat(p);
		  elem.pose.trans.z = GetFloat(p);
		  anm.data[bone].second.push_back(elem);
		}
	  }
	  result.animations.push_back(anm);
	}
  }

  return result;
}