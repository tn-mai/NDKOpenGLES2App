#include "Mesh.h"
#include <GLES2/gl2.h>
#include <algorithm>

//#define DEBUG_LOG_VERBOSE

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))
#else
#define LOGI(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#define LOGE(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#endif // __ANDROID__

namespace Mai {

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
	reserved          (2 byte)
	uint32_t          vbo offset by the top of file(32bit alignment).
	uint32_t          vbo byte size(32bit alignment).
	uint32_t          ibo byte size(32bit alignment).

	[
	  uint8_t         mesh name length.
	  char[mesh name length] mesh name(without zero ternmination).
	  uint8_t         material count.
	  padding         (4 - (length + 2) % 4) % 4 byte.
	  [
		uint32_t        ibo offset.
		uint16_t        ibo size(this is the polygon counts, so actual ibo size is 3 times).
		uint8_t         red
		uint8_t         green
		uint8_t         blue
		uint8_t         alpha
		uint8_t         metallic
		uint8_t         roughness
		] x (ibo count)
	] x (mesh count)

	uint8_t                          albedo texture name length.
	char[albedo texture name length] albedo texture name(without zero ternmination).
	uint8_t                          normal texture name length.
	char[normal texture name length] normal texture name(without zero ternmination).
	padding                          (4 - (texture name block size % 4) % 4 byte.

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
#ifdef SHOW_TANGENT_SPACE
  ImportMeshResult ImportMesh(const RawBuffer& data, GLuint& vbo, GLintptr& vboEnd, GLuint& ibo, GLintptr& iboEnd, GLuint vboTBN, GLintptr& vboTBNEnd)
#else
  ImportMeshResult ImportMesh(const RawBuffer& data, GLuint& vbo, GLintptr& vboEnd, GLuint& ibo, GLintptr& iboEnd)
#endif //  SHOW_TANGENT_SPACE
  {
	const uint8_t* p = &data[0];
	const uint8_t* pEnd = p + data.size();
	if (p[0] != 'M' || p[1] != 'S' || p[2] != 'H') {
	  return ImportMeshResult(ImportMeshResult::Result::invalidHeader);
	}
	p += 3;
	const int count = *p;
	p += 1;
	/*const uint32_t vboOffset = GetValue(p, 4);*/ p += 4;
	const uint32_t vboByteSize = GetValue(p, 4); p += 4;
	const uint32_t iboByteSize = GetValue(p, 4); p += 4;
	if (p >= pEnd) {
	  return ImportMeshResult(ImportMeshResult::Result::noData);
	}

	GLuint iboBaseOffset = iboEnd;
	ImportMeshResult  result(ImportMeshResult::Result::success);
	result.meshes.reserve(count);
	for (int i = 0; i < count; ++i) {
	  Mesh m;
	  const uint32_t nameLength = *p++;
	  m.id.assign(p, p + nameLength); p += nameLength;
	  const size_t materialCount = *p++;
	  p += (4 - (nameLength + 2) % 4) % 4;
	  m.materialList.resize(materialCount);
	  for (auto& e : m.materialList) {
		e.iboOffset = iboBaseOffset; p += 4;
		e.iboSize = GetValue(p, 2); p += 2;
		e.material.color.r = *p++;
		e.material.color.g = *p++;
		e.material.color.b = *p++;
		e.material.color.a = *p++;
		e.material.metallic.Set(static_cast<float>(*p++) / 255.0f);
		e.material.roughness.Set(static_cast<float>(*p++) / 255.0f);
		iboBaseOffset += e.iboSize * sizeof(GLushort);
	  }
	  result.meshes.push_back(m);
	  if (p >= pEnd) {
		return ImportMeshResult(ImportMeshResult::Result::invalidMeshInfo);
	  }
	}

#ifdef SHOW_TANGENT_SPACE
	const Vertex* pVBO = reinterpret_cast<const Vertex*>(reinterpret_cast<const void*>(p));
#endif // SHOW_TANGENT_SPACE

	glBufferSubData(GL_ARRAY_BUFFER, vboEnd, vboByteSize, p);
	p += vboByteSize;
	if (p >= pEnd) {
	  return ImportMeshResult(ImportMeshResult::Result::invalidVBO);
	}

#ifdef SHOW_TANGENT_SPACE
	const GLushort* pIBO = reinterpret_cast<const GLushort*>(reinterpret_cast<const void*>(p));
#endif // SHOW_TANGENT_SPACE

	std::vector<GLushort>  indices;
	indices.reserve(iboByteSize / sizeof(GLushort));
	const uint32_t offsetTmp = vboEnd / sizeof(Vertex);
	if (offsetTmp > 0xffff) {
	  return ImportMeshResult(ImportMeshResult::Result::indexOverflow);
	}
	const GLushort offset = static_cast<GLushort>(offsetTmp);
	for (uint32_t i = 0; i < iboByteSize; i += sizeof(GLushort)) {
	  indices.push_back(*reinterpret_cast<const GLushort*>(p) + offset);
	  p += sizeof(GLushort);
	  if (p >= pEnd) {
		return ImportMeshResult(ImportMeshResult::Result::invalidIBO);
	  }
	}
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, iboEnd, iboByteSize, &indices[0]);

#ifdef SHOW_TANGENT_SPACE
	if (vboTBN) {
	  for (auto& m : result.meshes) {
		std::vector<GLushort> indexList;
		indexList.reserve(3000);
		for (auto mm : m.materialList) {
		  const GLushort* p = pIBO + (mm.iboOffset - iboEnd) / sizeof(GLushort);
		  const GLushort* const end = p + mm.iboSize;
		  indexList.insert(indexList.end(), p, end);
		}
		std::sort(indexList.begin(), indexList.end());
		indexList.erase(std::unique(indexList.begin(), indexList.end()), indexList.end());

		const size_t tbnCount = indexList.size() * 3 * 2;
		if (vboTBNEnd + tbnCount * sizeof(TBNVertex) < vboTBNBufferSize) {
		  m.vboTBNOffset = vboTBNEnd / sizeof(TBNVertex);
		  m.vboTBNCount = tbnCount;
		} else {
		  m.vboTBNOffset = 0;
		  m.vboTBNCount = 0;
		  continue;
		}

		static const Color4B red(255, 0, 0, 255), green(0, 255, 0, 255), blue(0, 0, 255, 255);
		static const float lentgh = 0.2f;
		std::vector<TBNVertex> tbn;
		tbn.reserve(tbnCount);
		for (auto e : indexList) {
		  const Vertex& v = *(pVBO + e);
		  const Vector3F t = v.tangent.ToVec3();
		  const Vector3F b = v.normal.Cross(t).Normalize() * v.tangent.w;
		  tbn.push_back(TBNVertex(v.position, green, v.boneID, v.weight));
		  tbn.push_back(TBNVertex(v.position + b * lentgh, green, v.boneID, v.weight));
		  tbn.push_back(TBNVertex(v.position, red, v.boneID, v.weight));
		  tbn.push_back(TBNVertex(v.position + t * lentgh, red, v.boneID, v.weight));
		  tbn.push_back(TBNVertex(v.position, blue, v.boneID, v.weight));
		  tbn.push_back(TBNVertex(v.position + v.normal * lentgh, blue, v.boneID, v.weight));
		}
		glBindBuffer(GL_ARRAY_BUFFER, vboTBN);
		glBufferSubData(GL_ARRAY_BUFFER, vboTBNEnd, tbn.size() * sizeof(TBNVertex), &tbn[0]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		vboTBNEnd += tbn.size() * sizeof(TBNVertex);
	  }
	} else {
	  for (auto& m : result.meshes) {
		m.vboTBNOffset = 0;
		m.vboTBNCount = 0;
	  }
	}
#endif // SHOW_TANGENT_SPACE

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
	  for (uint32_t i = 0; i < boneCount; ++i) {
		if (p >= pEnd) {
		  return ImportMeshResult(ImportMeshResult::Result::invalidJointInfo);
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
	  for (uint32_t i = 0; i < boneCount; ++i) {
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
	  LOGI("ImportMesh - Read animation:");
	  result.animations.reserve(animationCount);
	  for (uint32_t i = 0; i < animationCount; ++i) {
		Animation anm;
		const uint32_t nameLength = GetValue(p++, 1);
		char name[24];
		for (int i = 0; i < 24; ++i) {
		  name[i] = static_cast<char>(GetValue(p++, 1));
		}
		anm.id.assign(name, name + nameLength);
		anm.data.resize(boneCount);
		for (uint32_t bone = 0; bone < boneCount; ++bone) {
		  anm.data[bone].first = bone;
		}
		anm.loopFlag = static_cast<bool>(GetValue(p++, 1) != 0);
		const uint32_t keyframeCount = GetValue(p, 2); p += 2;
		anm.totalTime = GetFloat(p);
		LOGI("%s: %fsec", anm.id.c_str(), anm.totalTime);
		for (uint32_t keyframe = 0; keyframe < keyframeCount; ++keyframe) {
		  const float time = GetFloat(p);
#ifdef DEBUG_LOG_VERBOSE
		  LOGI("time=%f", time);
#endif // DEBUG_LOG_VERBOSE
		  for (uint32_t bone = 0; bone < boneCount; ++bone) {
			if (p >= pEnd) {
			  return ImportMeshResult(ImportMeshResult::Result::invalidAnimationInfo);
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
#ifdef DEBUG_LOG_VERBOSE
			LOGI("%02d:(%+1.3f, %+1.3f, %+1.3f, %+1.3f) (%+1.3f, %+1.3f, %+1.3f)", bone, elem.pose.rot.w, elem.pose.rot.x, elem.pose.rot.y, elem.pose.rot.z, elem.pose.trans.x, elem.pose.trans.y, elem.pose.trans.z);
#endif // DEBUG_LOG_VERBOSE
		  }
		}
		result.animations.push_back(anm);
	  }
	}

	return result;
  }

} // namespace Mai