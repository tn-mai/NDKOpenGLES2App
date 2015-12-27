#include "TangentSpaceData.h"

namespace Mai {

  namespace TangentSpaceData {

	/// テクスチャ領域のUV展開方法.
	enum Type {
	  Type_Sphere, ///< 球・シリンダー式のUV展開で作成されたテクスチャ領域.
	  Type_Plane, ///< 通常のUV展開で作成されたテクスチャ領域.
	  Type_Cylinder0,
	  Type_Cylinder1,
	  Type_Terminator, ///< データ終端を示す特別な値.
	};

#define TERMINATOR { Vector2F(), Vector2F(), Type_Terminator, { Vector3F(), Vector3F() } }

	const Data defaultData[] = {
	  { Vector2F(0, 0), Vector2F(2, 2), Type_Plane, { Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  TERMINATOR
	};

	const Data sphereData[] = {
	  { Vector2F(0, 0), Vector2F(2, 2), Type_Sphere, { Vector3F(0, 1, 0), Vector3F(0, 0.99f, 0.14f) } },
	  TERMINATOR
	};

	const Data flyingrockData[] = {
	  { Vector2F(0.0f / 256.0f, 0.0f / 256.0f), Vector2F(87.0f / 256.0f, 1 - 119.0f / 256.0f), Type_Plane, { Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(0.0f / 256.0f, 0.0f / 256.0f), Vector2F(23.0f / 256.0f, 1 - 102.0f / 256.0f), Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(0, 0), Vector2F(2, 2), Type_Sphere, { Vector3F(0, 1, 0), Vector3F(0, 0.99f, 0.14f) } },
	  TERMINATOR
	};

	const Data block1Data[] = {
  #if 0
	  { Vector2F(0, 0), Vector2F(280.0f / 512.0f, 22.5f / 256.0f), Type_Cylinder0,{ Vector3F(1, 1, 0), Vector3F(1, 1, 0.2f) } },
	  { Vector2F(219.0f / 512.0f, 231.0f / 256.0f), Vector2F(1, 1), Type_Cylinder1,{ Vector3F(1, 1, 0), Vector3F(1, 1, 0.2f) } },
  #else
	  { Vector2F(0, 0), Vector2F(280.0f / 512.0f, 23.0f / 256.0f), Type_Sphere,{ Vector3F(0, 1, 0), Vector3F(0, 1, 0.2f) } },
	  { Vector2F(221.0f / 512.0f, 231.0f / 256.0f), Vector2F(1, 1), Type_Sphere,{ Vector3F(0, 1, 0), Vector3F(0, 1, 0.2f) } },
  #endif

	  //	{ Vector2F(0, 100.0f / 256.0f), Vector2F(1, 150.0f / 256.0f), Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  #if 1
		  { Vector2F(0, 0), Vector2F(0.5f, 1), Type_Plane, { Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
		  { Vector2F(0, 0), Vector2F(1.0f, 1), Type_Plane, { Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  #else
		  { Vector2F(0, 0), Vector2F(0.5f, 1), Type_Cylinder0,{ Vector3F(0, 1, 1), Vector3F(-0.99f, 0, 0.14f) } },
		  { Vector2F(0, 0), Vector2F(1.0f, 1), Type_Cylinder0,{ Vector3F(-1, 0, 0), Vector3F(-0.99f, 0, 0.14f) } },
	  #endif
		  TERMINATOR
	};

	const Data chickeneggData[] = {
	  { Vector2F(0 / 512.0f, 1 - 172.0f / 512.0f), Vector2F(236.0f / 512.0f, 2), Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(0 / 512.0f, 1 - 140.0f / 512.0f), Vector2F(260.0f / 512.0f, 2), Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(0 / 512.0f, 1 - 120.0f / 512.0f), Vector2F(2, 2),               Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(370.0f / 512.0f, 1 - 124.0f / 512.0f), Vector2F(2, 2),          Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(420.0f / 512.0f, 1 - 145.0f / 512.0f), Vector2F(2, 2),          Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(151.0f / 512.0f, 1 - 251.0f / 512.0f), Vector2F(187.0f / 512.0f, 1 - 172.0f / 512.0f), Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(163.0f / 512.0f, 0), Vector2F(199.0f / 512.0f, 1 - 430.0f / 512.0f), Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(0, 0), Vector2F(2, 2),                                          Type_Sphere,{ Vector3F(0, 1, 0), Vector3F(0, 0.99f, 0.14f) } },
	  TERMINATOR
	};

	const Data brokeneggData[] = {
	  { Vector2F(0 / 512.0f, 1 - 172.0f / 512.0f), Vector2F(236.0f / 512.0f, 2), Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(0 / 512.0f, 1 - 140.0f / 512.0f), Vector2F(260.0f / 512.0f, 2), Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(0 / 512.0f, 1 - 120.0f / 512.0f), Vector2F(2, 2),               Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(370.0f / 512.0f, 1 - 124.0f / 512.0f), Vector2F(2, 2),          Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(420.0f / 512.0f, 1 - 145.0f / 512.0f), Vector2F(2, 2),          Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  { Vector2F(0, 0), Vector2F(2, 2),                                          Type_Sphere,{ Vector3F(0, 1, 0), Vector3F(0, 0.99f, 0.14f) } },
	  TERMINATOR
	};

	const Data flyingpanData[] = {
	  { Vector2F(0, 0), Vector2F(2, 2), Type_Plane, { Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  TERMINATOR
	};

	const Data sunnysideupData[] = {
	  { Vector2F(0, 0), Vector2F(2, 2), Type_Plane, { Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  TERMINATOR
	};

	const Data acceleratorData[] = {
	  { Vector2F(0, 1 - 71.0f / 256.0f), Vector2F(2, 2), Type_Sphere,{ Vector3F(0, 1, 0), Vector3F(0, 0.99f, 0.14f) } },
	  { Vector2F(0, 0), Vector2F(2, 2), Type_Plane,{ Vector3F(1, 0, 0), Vector3F(0.99f, 0, 0.14f) } },
	  TERMINATOR
	};

	const Data* Find(const Data* data, const Position2S& s) {
	  const Position2F v = s.ToFloat();
	  while (data->type != Type_Terminator) {
		if (v.x >= data->lt.x && v.x <= data->rb.x && v.y >= data->lt.y && v.y <= data->rb.y) {
		  return data;
		}
		++data;
	  }
	  return nullptr;
	}

	void CalculateTangent(const Data* data, const std::vector<GLushort>& indices, int32_t vboEnd, std::vector<Vertex>& vertecies) {
#if 1
	  for (auto& e : vertecies) {
		if (auto p = Find(data, e.texCoord[0])) {
		  const Vector3F n(e.normal.x, e.normal.y, e.normal.z);
		  Vector3F t = n.Cross(Vector3F(p->tangent[0]).Normalize());
		  if (t.LengthSq() < FLT_EPSILON) {
			t = n.Cross(Vector3F(p->tangent[1]).Normalize());
		  }
		  t.Normalize();
		  const Vector3F b = n.Cross(t).Normalize();
		  t = n.Cross(b).Normalize();
		  float w = 1;
		  if (p->type == Type_Sphere) {
			if (std::abs(n.z) < 0.0001f) {
			  if (n.x == 0.0f) {
				w = n.y < 0.0f ? -1.0f : 1.0f;
			  } else {
				t.y = n.x < 0.0f ? -n.y : n.y;
				t.Normalize();
			  }
			} else {
			  w = n.z > 0.0f ? -1.0f : 1.0f;
			}
		  } else if (p->type == Type_Plane) {
			Vector3F b = n.Cross(Vector3F(p->tangent[0]).Normalize());
			if (b.LengthSq() < FLT_EPSILON) {
			  b = n.Cross(Vector3F(p->tangent[1]).Normalize());
			}
			b.Normalize();
			t = b.Cross(n).Normalize();
			w = -1.0f;
		  } else if (p->type == Type_Cylinder0) {
			Vector3F b = n.Cross(Vector3F(p->tangent[0]).Normalize());
			if (b.LengthSq() < FLT_EPSILON) {
			  b = n.Cross(Vector3F(p->tangent[1]).Normalize());
			}
			b.Normalize();
			t = b.Cross(n).Normalize();
		  } else if (p->type == Type_Cylinder1) {
			Vector3F b = n.Cross(Vector3F(p->tangent[0]).Normalize());
			if (b.LengthSq() < FLT_EPSILON) {
			  b = n.Cross(Vector3F(p->tangent[1]).Normalize());
			}
			b.Normalize();
			t = -b.Cross(n).Normalize();
			w = n.y > 0.0f ? -1.0f : 1.0f;
		  }
		  e.tangent = Vector4F(t, w);
		} else {
		  e.tangent = Vector4F(1, 0, 0, 1);
		}
	  }
#else
	  struct TangentInfo {
		Vector3F t;
		Vector3F b;
		TangentInfo() : t(0, 0, 0), b(0, 0, 0) {}
		TangentInfo(Vector3F t0, Vector3F& b0) : t(t0), b(b0) {}
	  };
	  std::vector<TangentInfo> tangentList;
	  tangentList.resize(vertecies.size());
	  const size_t end = indices.size();
	  for (size_t i = 0; i < end; i += 3) {
		const int i0 = indices[i + 0] - vboEnd / sizeof(Vertex);
		const int i1 = indices[i + 1] - vboEnd / sizeof(Vertex);
		const int i2 = indices[i + 2] - vboEnd / sizeof(Vertex);
		Vertex& a = vertecies[i0];
		Vertex& b = vertecies[i1];
		Vertex& c = vertecies[i2];
		const Position2F stA = a.texCoord[0].ToFloat();
		const Position2F stB = b.texCoord[0].ToFloat();
		const Position2F stC = c.texCoord[0].ToFloat();
		const Vector3F ab = b.position - a.position;
		const Vector3F ac = c.position - a.position;
		const float s1 = stB.x - stA.x;
		const float t1 = stB.y - stA.y;
		const float s2 = stC.x - stA.x;
		const float t2 = stC.y - stA.y;

		const float st_cross = s1 * t2 - t1 * s2;
		const float r = (std::abs(st_cross) <= 0.00001f) ? 1.0f : (1.0f / st_cross);
		Vector3F sdir(t2 * ab.x - t1 * ac.x, t2 * ab.y - t1 * ac.y, t2 * ab.z - t1 * ac.z);
		sdir *= r;
		sdir.Normalize();
		Vector3F tdir(s1 * ac.x - s2 * ab.x, s1 * ac.y - s2 * ab.y, s1 * ac.z - s2 * ab.z);
		tdir *= r;
		tdir.Normalize();
		tangentList[i0].t += sdir;
		tangentList[i0].b += tdir;
		tangentList[i1].t += sdir;
		tangentList[i1].b += tdir;
		tangentList[i2].t += sdir;
		tangentList[i2].b += tdir;
	  }

	  {
		typedef std::pair<Vertex*, TangentInfo*> VTPair;
		std::vector<VTPair> pairList;
		pairList.reserve(vertecies.size());
		auto tangentListItr = tangentList.begin();
		for (auto& e : vertecies) {
		  pairList.push_back(std::make_pair(&e, &*tangentListItr));
		  ++tangentListItr;
		}
		const auto end1 = pairList.end();
		const auto end0 = end1 - 1;
		std::vector<TangentInfo*> buf;
		buf.reserve(32);
		for (auto itr0 = pairList.begin(); itr0 != end0; ++itr0) {
		  buf.clear();
		  for (auto itr1 = itr0 + 1; itr1 != end1; ++itr1) {
			if (itr0->first->position == itr1->first->position && itr0->first->normal == itr1->first->normal) {
			  buf.push_back(itr1->second);
			}
		  }
		  if (!buf.empty()) {
			TangentInfo t = std::accumulate(
			  buf.begin(),
			  buf.end(),
			  *itr0->second,
			  [](TangentInfo& v, const TangentInfo* e) { return TangentInfo(v); }
			);
			for (auto e : buf) { *e = t; }
			*itr0->second = t;
		  }
		}
	  }

	  auto tangentListItr = tangentList.begin();
	  for (auto& e : vertecies) {
		const Vector3F n(e.normal.x, e.normal.y, e.normal.z);
		const Vector3F& t = tangentListItr->t;
		const Vector3F& b = tangentListItr->b;
		e.tangent = (t - n * Dot(n, t)).Normalize();
		e.tangent.w = Dot(Cross(t, b), b) < 0.0f ? -1.0f : 1.0f;
		++tangentListItr;
	  }
#endif
#if 0
	  const auto end1 = vertecies.end();
	  const auto end0 = end1 - 1;
	  std::vector<Vertex*> buf;
	  buf.reserve(1000);
	  for (auto itr0 = vertecies.begin(); itr0 != end0; ++itr0) {
		buf.clear();
		for (auto itr1 = itr0 + 1; itr1 != end1; ++itr1) {
		  if (itr0->position == itr1->position && itr0->normal == itr1->normal) {
			buf.push_back(&*itr1);
		  }
		}
		if (!buf.empty()) {
		  Vector4F t = std::accumulate(
			buf.begin(),
			buf.end(),
			itr0->tangent,
			[](Vector4F v, const Vertex* e) { return e->tangent + v; }
		  );
		  const float tmp = t.w;
		  t.w = 0;
		  t.Normalize();
		  t.w = tmp < 0.0f ? -1.0f : 1.0f;
		  for (auto e : buf) { e->tangent = t; }
		  itr0->tangent = t;
		}
	  }
#endif
	}
  } // namespace TangentSpaceData

} // namespace Mai