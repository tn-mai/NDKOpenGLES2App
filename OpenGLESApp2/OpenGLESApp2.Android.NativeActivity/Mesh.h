#ifndef FBX_H_INCLUDED
#define FBX_H_INCLUDED

#include "Renderer.h"
#include <vector>

namespace Mai {

  typedef std::vector<uint8_t> RawBuffer;
  typedef std::vector<Vertex> VertexBuffer;
  typedef std::vector<GLshort> IndexBuffer;

  struct ImportMeshResult {
	enum class Result {
	  success,
	  invalidHeader,
	  noData,
	  invalidMeshInfo,
	  invalidIBO,
	  invalidVBO,
	  invalidJointInfo,
	  invalidAnimationInfo,
	  indexOverflow,
	};
	explicit ImportMeshResult(Result r) : result(r) {}

	Result result;
	std::vector<Mesh> meshes;
	std::vector<Animation> animations;
  };

#ifdef SHOW_TANGENT_SPACE
  ImportMeshResult ImportMesh(const RawBuffer& data, GLuint& vbo, GLintptr& vboEnd, GLuint& ibo, GLintptr& iboEnd, GLuint vboTBN, GLintptr& vboTBNEnd);
#else
  ImportMeshResult ImportMesh(const RawBuffer& data, GLuint& vbo, GLintptr& vboEnd, GLuint& ibo, GLintptr& iboEnd);
#endif // SHOW_TANGENT_SPACE

} // namespace Mai

#endif // FBX_H_INCLUDED