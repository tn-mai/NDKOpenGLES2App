#ifndef FBX_H_INCLUDED
#define FBX_H_INCLUDED

#include "Renderer.h"
#include <vector>

typedef std::vector<uint8_t> RawBuffer;
typedef std::vector<Vertex> VertexBuffer;
typedef std::vector<GLshort> IndexBuffer;

struct ImportMeshResult {
  std::vector<Mesh> meshes;
  std::vector<Animation> animations;
};

ImportMeshResult ImportMesh(const RawBuffer& data, GLuint& vbo, GLintptr& vboEnd, GLuint& ibo, GLintptr& iboEnd);

#endif // FBX_H_INCLUDED