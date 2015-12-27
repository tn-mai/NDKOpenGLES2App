#ifndef TANGENTSPACEDATA_H_INCLUDED
#define TANGENTSPACEDATA_H_INCLUDED
#include "Renderer.h"

namespace Mai {

  namespace TangentSpaceData {

	struct Data {
	  Vector2F lt, rb;
	  int type;
	  Vector3F tangent[2];
	};

	const Data* Find(const Data* data, const Position2S& s);
	void CalculateTangent(const Data* data, const std::vector<GLushort>& indices, int32_t vboEnd, std::vector<Vertex>& vertecies);

	extern const Data defaultData[];
	extern const Data sphereData[];
	extern const Data flyingrockData[];
	extern const Data block1Data[];
	extern const Data chickeneggData[];
	extern const Data flyingpanData[];
	extern const Data sunnysideupData[];
	extern const Data brokeneggData[];
	extern const Data acceleratorData[];

  } // namespace TangentSpaceData

} // namespace Mai

#endif // TANGENTSPACEDATA_H_INCLUDED