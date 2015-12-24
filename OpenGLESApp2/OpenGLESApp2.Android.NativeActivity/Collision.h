#ifndef COLLISION_H_INCLUDED
#define COLLISION_H_INCLUDED
#include "Renderer.h"
#include <vector>
#include <map>
#include <stdint.h>

namespace Collision {

  struct Point {
	explicit Point(const Position3F& p) : point(p) {}

	Position3F point;
  };

  struct Line {
	Line(const Position3F& a, const Vector3F& b) : point(a), vec(b) {}
	Position3F EndPoint0() const { return point; }
	Position3F EndPoint1() const { return point + vec; }
	float LengthSq() const { return vec.LengthSq(); }
	
	Position3F point;
	Vector3F vec;
  };

  struct Sphere {
	Sphere(const Position3F& p, float r) : center(p), radius(r) {}

	Position3F center;
	float radius;
  };

  struct Plane {
	Plane() {}
	Plane(const Position3F& p, const Vector3F& n) : point(p), normal(n) {}
	float GetD() const { return normal.Dot(Vector3F(point)); }
	float GetD(float r) const { return normal.Dot(Vector3F(point) + normal * r); }
	Position3F point;
	Vector3F normal;
  };

  struct StraightLine {
	StraightLine(const Position3F& p, const Vector3F& n) : point(p), vector(n) {}

	Position3F point;
	Vector3F vector;
  };

  struct Triangle {
	Triangle(const Position3F& p0, const Position3F& p1, const Position3F& p2) : point{ p0, p1, p2 } {}

	Position3F point[3];
	Vector3F normal;
  };

  enum ShapeID {
	ShapeID_Sphere = 0,
	ShapeID_Plane = 1,
	ShapeID_Box = 2,
  };

  struct AABB {
	Vector3F a, b;
  };

  struct RigidBody {
	RigidBody(ShapeID sid, float kg, int mid = 0)
	  : shapeID(sid)
	  , materialID(mid)
	  , hasLatestCollision(false)
	  , isStatic(true)
	  , m(kg)
	  , thrust(Vector3F::Unit())
	  , accel(Vector3F::Unit())
	  , innerEnergy(0)
	{}
	virtual ~RigidBody() = 0;
	virtual void Move(Vector3F v) = 0;
	virtual Position3F Position() const = 0;
	virtual void Position(const Position3F&) = 0;
	virtual Vector3F ApplyRotation(const Vector3F& pos) const { return pos; }
	void SetAccel(Vector3F);

	uint8_t shapeID;

	uint8_t materialID;
	bool hasLatestCollision;
	bool isStatic;
	float m;
	Vector3F thrust;

	Vector3F accel;
	AABB aabb;

	float innerEnergy;
	Vector3F v;
  };
  inline  RigidBody::~RigidBody() = default;
  typedef std::shared_ptr<RigidBody> RigidBodyPtr;

  struct SphereShape : public RigidBody {
	explicit SphereShape(const Position3F& p, float r, float m) : RigidBody(ShapeID_Sphere, m), shape(p, r) {}
	virtual ~SphereShape() {}
	virtual void Move(Vector3F v) { shape.center += v; }
	virtual Position3F Position() const { return shape.center; }
	virtual void Position(const Position3F& pos) { shape.center = pos; }

	Sphere shape;
  };

  struct PlaneShape : public RigidBody {
	explicit PlaneShape(const Position3F& p, const Vector3F& n, float m) : RigidBody(ShapeID_Plane, m), shape(p, n) {}
	virtual ~PlaneShape() {}
	virtual void Move(Vector3F v) { shape.point += v; }
	virtual Position3F Position() const { return shape.point; }
	virtual void Position(const Position3F& pos) { shape.point = pos; }

	Plane shape;
  };

  struct BoxShape : public RigidBody {
	explicit BoxShape(const Position3F& o, const Quaternion& q, const Vector3F& s, float m)
	  : RigidBody(ShapeID_Box, m)
	  , center(o)
	  , normal{ q.RightVector(), q.UpVector(), q.ForwardVector() }
	  , scale(s)
	{
	}
	virtual ~BoxShape() {}
	virtual void Move(Vector3F v) {}
	virtual Position3F Position() const { return center; }
	virtual void Position(const Position3F& p) { center = p; }
	virtual Vector3F ApplyRotation(const Vector3F& pos) const {
	  return Vector3F(normal[0].Dot(pos), normal[1].Dot(pos), normal[2].Dot(pos));
	}

	Position3F center;
	Vector3F normal[3];
	Vector3F scale;
  };

  struct CollisionInfo {
	Plane p; // contact point and normal.
	RigidBodyPtr bodies[2];
  };

  class World {
  public:
	World() : g(9.8f) {}
	void Insert(const RigidBodyPtr&);
	void Erase(const RigidBodyPtr&);
	void Clear();
	void Step(float);
  private:
	std::vector<RigidBodyPtr> bodies;
	float g;
  };

} // namespace Collision


#endif // COLLISION_H_INCLUDED