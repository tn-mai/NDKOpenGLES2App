#include "Collision.h"
#include <algorithm>
#include <list>
#include <boost/optional.hpp>

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))
#else
#include <stdio.h>
#define LOGI(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#define LOGE(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#endif // __ANDROID__

namespace Mai {

  namespace Collision {

	typedef boost::optional<Plane> Result;

	void RigidBody::SetAccel(Vector3F v) {
	  //	const float y = v.y;
	  //	if (std::isnan(y)) {
	  //	  v.y = 0;
	  //	}
	  accel = v;
	}

	void SolveRestitution(RigidBody& lhs, RigidBody& rhs, const Plane& plane, float t) {
	  static const float e = 0.25f; // coefficient of restitution.

	  const float a1 = plane.normal.Dot(lhs.accel);
	  const float a2 = plane.normal.Dot(rhs.accel);
	  const float v1 = ((lhs.m - rhs.m) * a1 + 2.0f * rhs.m * a2) / (lhs.m + rhs.m);
	  const float v2 = ((rhs.m - lhs.m) * a2 + 2.0f * lhs.m * a1) / (lhs.m + rhs.m);
	  Vector3F rv1 = lhs.accel + plane.normal * (v1 * e - a1);
	  if (rv1.LengthSq() <= 0.00000001) {
		rv1 = Vector3F::Unit();
	  }
	  Vector3F rv2 = rhs.accel + plane.normal * (v2 * e - a2);
	  if (rv2.LengthSq() <= 0.00000001) {
		rv2 = Vector3F::Unit();
	  }
	  lhs.innerEnergy += std::abs(rhs.accel.Length() - rv2.Length()) * rhs.m; // lhs get the lost energy of rhs.
	  rhs.innerEnergy += std::abs(lhs.accel.Length() - rv1.Length()) * lhs.m; // rhs get the lost energy of lhs.
	  lhs.SetAccel(rv1);
	  rhs.SetAccel(rv2);
	  lhs.v = Vector3F(lhs.accel).Normalize() * lhs.v.Length() * (1 - std::max(t, 0.1f));
	  if (lhs.v.LengthSq() <= 0.00000001) {
		lhs.v = Vector3F::Unit();
	  }
	  rhs.v = Vector3F(rhs.accel).Normalize() * rhs.v.Length() * (1 - std::max(t, 0.1f));
	  if (rhs.v.LengthSq() <= 0.00000001) {
		rhs.v = Vector3F::Unit();
	  }
	}

	boost::optional<float> IntersectRaySphere(const StraightLine& l, const Sphere& s) {
	  const Vector3F m = l.point - s.center;
	  const float b = m.Dot(l.vector);
	  const float c = m.LengthSq() - s.radius * s.radius;
	  if (c > 0.0f && b > 0.0f) {
		return boost::optional<float>();
	  }
	  const float discr = b * b - c;
	  if (discr < 0.0f) {
		return boost::optional<float>();
	  }
	  return boost::optional<float>(-b - std::sqrt(discr));
	}

	float IntersectSegmentPlane(const Line& line, const Plane& plane) {
	  const float distance = plane.normal.Dot(line.point - plane.point);
	  const float denom = plane.normal.Dot(line.vec);
	  return -distance / denom;
	}

	Result SphereSphere(RigidBody& lhs, RigidBody& rhs) {
	  SphereShape& sphereA = static_cast<SphereShape&>(lhs);
	  SphereShape& sphereB = static_cast<SphereShape&>(rhs);

	  const float r = sphereA.shape.radius + sphereB.shape.radius;
	  const Vector3F d = (sphereA.v - sphereB.v).Normalize();
	  if (auto irs = IntersectRaySphere(StraightLine(sphereA.shape.center, d), Sphere(sphereB.shape.center, r))) {
		const float t = irs.get();
		if (t > 1.0f) {
		  return Result();
		}
		if (t < 0.0f) {
		  // 重なっていた場合の対処.
		  const Vector3F m = sphereB.shape.center - sphereA.shape.center;
		  const Vector3F n = Vector3F(m).Normalize();
		  const float c = m.LengthSq() - r * r;
		  const float f = std::min(0.001f * c, 0.001f);
		  sphereA.shape.center += n * f;
		  sphereB.shape.center -= n * f;
		  return Result();// Result(Plane(sphereB.shape.center + n * (sphereB.shape.radius / r), n));
		}
		sphereA.shape.center += sphereA.v * t;
		sphereB.shape.center += sphereB.v * t;
		const Vector3F n = sphereA.shape.center - sphereB.shape.center;
		const Position3F p = sphereB.shape.center + n * sphereB.shape.radius / r;
		const Plane plane(p, Vector3F(n).Normalize());

		SolveRestitution(lhs, rhs, plane, t);
		return Result(plane);
	  }
	  return Result();
	}

	Result SpherePlane(RigidBody& lhs, RigidBody& rhs) {
	  SphereShape& sphere = static_cast<SphereShape&>(lhs);
	  PlaneShape& plane = static_cast<PlaneShape&>(rhs);

	  float t;
	  Plane planeA;
	  const float distance = plane.shape.normal.Dot(sphere.shape.center - plane.shape.point);
	  const Vector3F v = sphere.v - plane.v;
	  const float denom = plane.shape.normal.Dot(v);
	  // 平行あるいは遠ざかるように移動している場合、衝突は起こらない.
	  if (denom * distance >= 0.0f) {
		return Result();
	  }
	  const float c = std::abs(distance);
	  if (c < sphere.shape.radius) {
		// すでに重なっている.
		const float f = std::min(0.001f * c, 0.001f);
		sphere.v += plane.shape.normal * f;
		t = 0.0f;
		planeA.point = sphere.shape.center - plane.shape.normal * distance;
	  } else {
		const float r = distance >= 0.0f ? sphere.shape.radius : -sphere.shape.radius;
		t = (r - distance) / denom;
		if (t > 1.0f) {
		  return Result();
		}
		sphere.shape.center += sphere.v * t;
		plane.shape.point += plane.v * t;
		planeA.point = sphere.shape.center - r * plane.shape.normal;
	  }
	  planeA.normal = distance > 0.0f ? plane.shape.normal : -plane.shape.normal;
	  SolveRestitution(lhs, rhs, planeA, t);
	  return Result(planeA);
	}
	Result PlaneSphere(RigidBody& lhs, RigidBody& rhs) { return SpherePlane(rhs, lhs); }

	float DistanceSquared(const Line& s0, const Line& s1, float& sc, float& tc) {
	  const Vector3F w0 = s0.point - s1.point;
	  const float s0lensq = s0.vec.LengthSq();
	  const float s1lensq = s1.vec.LengthSq();
	  const float b = s0.vec.Dot(s1.vec);
	  const float invD = -s0.vec.Dot(w0);
	  const float e = s1.vec.Dot(w0);

	  const float denom = s0lensq * s1lensq - b * b;
	  float sn, sd, tn, td;
	  if (denom < FLT_EPSILON) {
		sd = td = s1lensq;
		sn = 0.0f;
		tn = e;
	  } else {
		sd = denom;
		sn = b * e + s1lensq * invD;
		if (sn < 0.0f) {
		  sn = 0.0f;
		  tn = e;
		  td = s1lensq;
		} else if (sn > sd) {
		  sn = sd;
		  tn = e + b;
		  td = s1lensq;
		} else {
		  tn = s0lensq * e + b * invD;
		  td = denom;
		}
	  }

	  if (tn < 0.0f) {
		tc = 0.0f;
		if (invD < 0.0f) {
		  sc = 0.0f;
		} else if (invD > s0lensq) {
		  sc = 1.0f;
		} else {
		  sc = invD / s0lensq;
		}
	  } else if (tn > td) {
		tc = 1.0f;
		const float tmp = invD + b;
		if (tmp < 0.0f) {
		  sc = 0.0f;
		} else if (tmp > s0lensq) {
		  sc = 1.0f;
		} else {
		  sc = tmp / s0lensq;
		}
	  } else {
		tc = tn / td;
		sc = sn / sd;
	  }
	  return (w0 + sc * s0.vec - tc * s1.vec).LengthSq();
	}

	float ClosestPointLine(const Position3F& c, const Position3F& a, const Vector3F& ab) {
	  return (c - a).Dot(ab) / ab.Dot(ab);
	}

	struct Capsule {
	  Capsule(const Position3F& a, const Position3F& b, float rr) : n(a, b - a), r(rr) {}
	  Line n;
	  float r;
	};
	struct ResultISC {
	  ResultISC() : hasCollision(false) {}
	  ResultISC(bool i, float tt, const Vector3F& nn) : hasCollision(true), isInner(i), t(tt), n(nn) {}
	  operator bool() const { return hasCollision; }
	  bool hasCollision;
	  bool isInner;
	  float t;
	  Vector3F n;
	};
	ResultISC IntersectSegmentSphere(const Line& l, const Sphere& s) {
	  const Vector3F vn = Vector3F(l.vec).Normalize();
	  if (auto irs = IntersectRaySphere(StraightLine(l.point, vn), s)) {
		const float t = irs.get();
		if (t < 0.0f) {
		  const Vector3F m = l.point - s.center;
		  const Vector3F n = Vector3F(m).Normalize();
		  return ResultISC(true, 0.0f, n);
		} else if (t <= 1.0f) {
		  const Position3F x = l.point + t * vn;
		  return ResultISC(false, t, (x - s.center).Normalize());
		}
	  }
	  return ResultISC();
	}
	ResultISC IntersectSegmentCapsule(const Line& l, const Capsule& capsule) {
	  const Vector3F m = l.point - capsule.n.point;
	  const float md = m.Dot(capsule.n.vec);
	  const float nd = l.vec.Dot(capsule.n.vec);
	  const float dd = capsule.n.vec.Dot(capsule.n.vec);
	  if (md < 0.0f && md + nd < 0.0f) {
		return IntersectSegmentSphere(l, Sphere(capsule.n.point, capsule.r));
	  } else if (md < dd && md + nd > dd) {
		return IntersectSegmentSphere(l, Sphere(capsule.n.point + capsule.n.vec, capsule.r));
	  }

	  const float nn = l.vec.Dot(l.vec);
	  const float mn = m.Dot(l.vec);
	  const float a = dd * nn - nd * nd;
	  const float k = m.Dot(m) - capsule.r * capsule.r;
	  const float c = dd * k - md * md;
	  if (std::abs(a) < FLT_EPSILON) {
		if (c > 0.0f) {
		  return ResultISC();
		}
		if (md < 0.0f) {
		  return IntersectSegmentSphere(l, Sphere(capsule.n.point, capsule.r));
		} else if (md > dd) {
		  return IntersectSegmentSphere(l, Sphere(capsule.n.point + capsule.n.vec, capsule.r));
		}
		const float t0 = ClosestPointLine(l.point, capsule.n.point, capsule.n.vec);
		const Position3F p0 = capsule.n.point + capsule.n.vec * t0;
		return ResultISC(true, 0.0f, (l.point - p0).Normalize());
	  }

	  const float b = dd * mn - nd * md;
	  const float discr = b * b - a * c;
	  if (discr < 0.0f) {
		return ResultISC();
	  }
	  const float t = (-b - std::sqrt(discr)) / a;
	  if (t < 0.0f || t > 1.0f) {
		return ResultISC();
	  }
	  const float u = md + t * nd;
	  if (u < 0.0f) {
		return IntersectSegmentSphere(l, Sphere(capsule.n.point, capsule.r));
	  } else if (u > dd) {
		return IntersectSegmentSphere(l, Sphere(capsule.n.point + capsule.n.vec, capsule.r));
	  }

	  const Position3F x = l.point + l.vec * t;
	  const float t0 = ClosestPointLine(x, capsule.n.point, capsule.n.vec);
	  const Position3F p0 = capsule.n.point + capsule.n.vec * t0;
	  return ResultISC(false, t, (x - p0).Normalize());
	}

	Result SphereBox(RigidBody& lhs, RigidBody& rhs) {
	  SphereShape& sphere = static_cast<SphereShape&>(lhs);
	  BoxShape& box = static_cast<BoxShape&>(rhs);
	  const Vector3F v = sphere.v - box.v;
	  {
		const float dist = (sphere.shape.center - box.center).LengthSq();
		const float r = sphere.shape.radius + v.Length() + box.scale.Length();
		if (dist > r * r) {
		  return Result();
		}
	  }
	  const Vector3F so = sphere.shape.center - box.center;
	  float tmin = 0;
	  float tmax = 1;
	  float distance = 1;
	  Vector3F n = Vector3F::Unit();
	  const Line line(sphere.shape.center, v);
	  for (int i = 0; i < 3; ++i) {
		const float boxscale = (&box.scale.x)[i];
		const float vn = Dot(v, box.normal[i]);
		if (std::abs(vn) <= FLT_EPSILON) {
		  float dist = Dot(so, box.normal[i]);
		  if (dist >= 0.0f) {
			dist -= boxscale + sphere.shape.radius;
			if (dist > 0.0f) {
			  return Result();
			}
			if (-dist < distance) {
			  distance = -dist;
			  n = box.normal[i];
			}
		  } else {
			dist += boxscale + sphere.shape.radius;
			if (dist < 0.0f) {
			  return Result();
			}
			if (dist < distance) {
			  distance = dist;
			  n = -box.normal[i];
			}
		  }
		}
		const float t1 = IntersectSegmentPlane(line, Plane(box.center + box.normal[i] * (boxscale + sphere.shape.radius), box.normal[i]));
		const float t2 = IntersectSegmentPlane(line, Plane(box.center - box.normal[i] * (boxscale + sphere.shape.radius), -box.normal[i]));
		if (t1 <= t2) {
		  if (tmin < t1) {
			tmin = t1;
			n = box.normal[i];
		  }
		  tmax = std::min(tmax, t2);
		} else {
		  if (tmin < t2) {
			tmin = t2;
			n = -box.normal[i];
		  }
		  tmax = std::min(tmax, t1);
		}
		if (tmin > tmax) {
		  return Result();
		}
	  }

	  struct local {
		static Position3F Corner(const BoxShape& b, int n) {
		  Position3F p = b.center;
		  p += (n & 1 ? b.normal[0] : -b.normal[0]) * b.scale.x;
		  p += (n & 2 ? b.normal[1] : -b.normal[1]) * b.scale.y;
		  p += (n & 4 ? b.normal[2] : -b.normal[2]) * b.scale.z;
		  return p;
		}
	  };

	  bool logged = false;
	  int u0 = 0, u1 = 0;
	  const Vector3F p = sphere.shape.center + tmin * sphere.v - box.center;
	  const float f0 = box.normal[0].Dot(p);
	  if (f0 < -box.scale.x) u0 |= 1;
	  if (f0 > box.scale.x) u1 |= 1;
	  const float f1 = box.normal[1].Dot(p);
	  if (f1 < -box.scale.y) u0 |= 2;
	  if (f1 > box.scale.y) u1 |= 2;
	  const float f2 = box.normal[2].Dot(p);
	  if (f2 < -box.scale.z) u0 |= 4;
	  if (f2 > box.scale.z) u1 |= 4;
	  const int m = u0 | u1;
	  if (m == 7) {
		tmin = FLT_MAX;
		const Position3F c0 = local::Corner(box, u1);
		const Line l(sphere.shape.center, sphere.v);
		if (auto ret = IntersectSegmentCapsule(l, Capsule(c0, local::Corner(box, u1 ^ 1), sphere.shape.radius))) {
		  if (ret.t < tmin) {
			LOGI("Hit 0: (%f, %f, %f), %f, %f", ret.n.x, ret.n.y, ret.n.z, tmin, tmax);
			logged = true;
			tmin = ret.t;
			n = ret.n;
		  }
		}
		if (auto ret = IntersectSegmentCapsule(l, Capsule(c0, local::Corner(box, u1 ^ 2), sphere.shape.radius))) {
		  if (ret.t < tmin) {
			LOGI("Hit 1: (%f, %f, %f), %f, %f", ret.n.x, ret.n.y, ret.n.z, tmin, tmax);
			logged = true;
			tmin = ret.t;
			n = ret.n;
		  }
		}
		if (auto ret = IntersectSegmentCapsule(l, Capsule(c0, local::Corner(box, u1 ^ 4), sphere.shape.radius))) {
		  if (ret.t < tmin) {
			LOGI("Hit 2: (%f, %f, %f), %f, %f", ret.n.x, ret.n.y, ret.n.z, tmin, tmax);
			logged = true;
			tmin = ret.t;
			n = ret.n;
		  }
		}
		if (tmin == FLT_MAX) {
		  return Result();
		}
	  } else if (m & (m - 1)) {
		const Position3F c0 = local::Corner(box, u0 ^ 7);
		const Position3F c1 = local::Corner(box, u1);
		const Line l(sphere.shape.center, sphere.v);
		if (auto ret = IntersectSegmentCapsule(l, Capsule(c0, c1, sphere.shape.radius))) {
		  LOGI("Hit 3: (%f, %f, %f), %f, %f", ret.n.x, ret.n.y, ret.n.z, tmin, tmax);
		  logged = true;
		  tmin = ret.t;
		  n = ret.n;
		} else {
		  return Result();
		}
	  }

	  sphere.shape.center += sphere.v * tmin;
	  box.center += box.v * tmin;
	  if (n.x == 0 && n.y == 0 && n.z == 0) {
		switch (u0 + (u1 << 3)) {
		case 0x01: n = -box.normal[0]; break;
		case 0x02: n = -box.normal[1]; break;
		case 0x04: n = -box.normal[2]; break;
		case 0x08: n = box.normal[0]; break;
		case 0x10: n = box.normal[1]; break;
		case 0x20: n = box.normal[2]; break;
		default:
		  LOGI("Hit?:%x, %x", u0, u1);
		  return Result();
		}
	  }
	  if (!logged) {
		LOGI("Hit: (%f, %f, %f), %f, %f", n.x, n.y, n.z, tmin, tmax);
	  }
	  const Plane plane(sphere.shape.center - sphere.shape.radius * n, n);
	  SolveRestitution(lhs, rhs, plane, tmin);
	  return Result(plane);
	}
	Result BoxSphere(RigidBody& lhs, RigidBody& rhs) { return SphereBox(rhs, lhs); }

	Result BoxPlane(RigidBody& lhs, RigidBody& rhs) { return Result(); }
	Result PlaneBox(RigidBody& lhs, RigidBody& rhs) { return BoxPlane(rhs, lhs); }

	Result BoxBox(RigidBody& lhs, RigidBody& rhs) { return Result(); }

	Result NOTIMPLEMENT(RigidBody&, RigidBody&) { return Result(); }

	void World::Step(float delta) {
	  typedef Result(*FuncType)(RigidBody&, RigidBody&);
	  static const FuncType dispatcherList[3][3] = {
		// Sphere       Plane         Box
		{ SphereSphere, PlaneSphere,  BoxSphere }, // Sphere
		{ SpherePlane,  NOTIMPLEMENT, BoxPlane  }, // Plane
		{ SphereBox,    PlaneBox,     BoxBox    }, // Box
	  };
	  if (bodies.empty()) {
		return;
	  }
	  for (auto& p : bodies) {
		RigidBody& e = *p;
		Vector3F v = e.accel + e.thrust * delta;
		v.y -= g * delta;

		// solve air friction.
		// スカイダイビングの終端加速度は50m/s～85m/sらしい.
		// "Poynter, D. and Turoff, M., The Skydiver's Handbook, (2004), pp.191 - 193"に平均的な終端速度が書いてあるらしい.
		const float f = v.Length();
		if (f > FLT_EPSILON) {
		  static float Cd = 0.24f; // どこかでスカイダイバーの抗力係数は0.24くらいと書いてあった. 本当のところは不明.
		  // 空気密度pの近似式はここのを使っている @ref http://kinkink.web.fc2.com/contents/density/index.html
		  const float z = e.Position().y * 0.001f;
		  const float z2 = z * z;
		  const float p = -0.0000171f * z2 * z + 0.002978f * z2 - 0.10975f * z + 1.225f;
		  static float S = 1.0f;
		  const float D = 0.5f * p * f * f * S * Cd;
		  const float friction = D / (e.m * 1000.0f);
		  if (friction < f) {
			v -= Vector3F(v).Normalize() * friction * delta;
		  } else if (friction > f) {
			v.y += 0.0f;
		  }
		}
		e.SetAccel(v);
		e.v = e.accel * delta;
		e.innerEnergy = 0;
		e.hasLatestCollision = false;
	  }
	  RigidBody* pLatestCollider = nullptr;
	  for (auto itrL = bodies.begin(); itrL != bodies.end();) {
		RigidBody& lhs = *itrL->get();
		int collisionCount = 0;
		auto itrR = itrL;
		++itrR;
		for (; itrR != bodies.end(); ++itrR) {
		  RigidBody& rhs = *itrR->get();
		  if (&lhs == &rhs || pLatestCollider == &rhs) {
			continue;
		  }
		  if (auto r = dispatcherList[rhs.shapeID][lhs.shapeID](lhs, rhs)) {
			// 当たり方に応じて加速度を減らす(摩擦っぽい処理).
			const Vector3F& normal = r->normal;
			if (lhs.accel.LengthSq()) {
			  const float friction = 0.9f - Normalize(lhs.accel).Dot(normal) * 0.125f;
			  lhs.accel *= friction;
			}
			if (rhs.accel.LengthSq()) {
			  const float friction = 0.9f - Normalize(rhs.accel).Dot(normal) * 0.125f;
			  rhs.accel *= friction;
			}
			lhs.hasLatestCollision = true;
			rhs.hasLatestCollision = true;
			pLatestCollider = &rhs;
			++collisionCount;
			if (lhs.v.LengthSq() <= 0.0f) {
			  break;
			}
		  }
		}
		if (!collisionCount || lhs.v.LengthSq() <= 0.0f) {
		  lhs.Move(lhs.v);
		  lhs.v = Vector3F::Unit();
		  pLatestCollider = nullptr;
		  ++itrL;
		}
		// 重力による振動をおさえる.
		else if (collisionCount == 1 && lhs.accel.y > 0.0f && lhs.accel.y <= g * delta) {
		  const float v = lhs.accel.Length();
		  if (lhs.accel.y / v >= 0.8f) {
			lhs.accel = Vector3F::Unit();
			lhs.v = Vector3F::Unit();
			pLatestCollider = nullptr;
			++itrL;
		  }
		}
	  }
	}

	void World::Insert(const RigidBodyPtr& p) {
	  bodies.push_back(p);
	}

	void World::Erase(const RigidBodyPtr& p) {
	  auto itr = std::find(bodies.begin(), bodies.end(), p);
	  if (itr != bodies.end()) {
		bodies.erase(itr);
	  }
	}
	void World::Clear() {
	  bodies.clear();
	}

  } // namespace Collision

} // namespace Mai