/** @file MainGameScene.cpp
*/
#include "LevelInfo.h"
#include "Scene.h"
#include "../File.h"
#include "../Audio.h"
#include "../../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include <boost/math/constants/constants.hpp>
#include <array>
#include <tuple>
#include <algorithm>
#include <random>

// if activate this macro, the collision box of obstacles is displayed.
//#define SSU_DEBUG_DISPLAY_COLLISION_BOX

namespace SunnySideUp {

  using namespace Mai;

  enum DirectionKey {
	DIRECTIONKEY_UP,
	DIRECTIONKEY_LEFT,
	DIRECTIONKEY_DOWN,
	DIRECTIONKEY_RIGHT,
  };

  /** The additional function controller for debugging.

    This class uses only on debugging.
	It will disabled if NDEBUG macro is defined.
  */
  struct DebugData {
#ifndef NDEBUG
	DebugData()
	  : debug(false)
	  , dragging(false)
	  , mouseX(-1)
	  , mouseY(-1)
	  , camera(Position3F(0, 0, 0), Vector3F(0, 0, -1), Vector3F(0, 1, 0))
	{
#ifdef SSU_DEBUG_DISPLAY_COLLISION_BOX
	  collisionBoxList.reserve(64);
#endif // SSU_DEBUG_DISPLAY_COLLISION_BOX
	}

	/// Clear all holding objects.
	void Clear() {
#ifdef SSU_DEBUG_DISPLAY_COLLISION_BOX
	  collisionBoxList.clear();
#endif // SSU_DEBUG_DISPLAY_COLLISION_BOX
	  for (auto& e : debugObj) {
		e.reset();
	  }
	}

	/** Set the object to the debugging target.

	  @param i    The target index(0-2). Do nothing if it is invalid value.
	  @param obj  The object setting to (i)th debugging target.

	  If (i)th already set, it will be overwritten.
	*/
	void SetDebugObj(size_t i, const ObjectPtr& obj) {
	  if (i < debugObj.size()) {
		debugObj[i] = obj;
	  }
	}

	/** Add collision box.

	  @param r           The renderer object.
	  @param trans       The translation of collision.
	  @param rx, ry, rz  The rotation of collision.
	  @param scale       The scaling of collision.
	                     Box shape has 1x1x1 size(it means that each axis have 0.5 length).
						 So, (1, 1, 1) is given to scale, one vertex have (0.5, 0.5, 0.5) position.

	  Add the collision box to the collision box list.
	  The list is append by AppendCollsionBox() to the display object list.

	  @sa AppendCollisionBox().
	*/
#ifdef SSU_DEBUG_DISPLAY_COLLISION_BOX
	void AddCollisionBoxShape(Renderer& r, const Vector3F& trans, float rx, float ry, float rz, const Vector3F& scale) {
	  auto o = r.CreateObject("unitbox", Material(Color4B(255, 255, 255, 255), 0, 0), "default", ShadowCapability::Disable);
	  o->SetTranslation(trans);
	  o->SetRotation(rx, ry, rz);
	  o->SetScale(scale);
	  collisionBoxList.push_back(o);
	}
#else
	void AddCollisionBoxShape(Renderer&, const Vector3F&, float, float, float, const Vector3F&) {}
#endif // SSU_DEBUG_DISPLAY_COLLISION_BOX

	/** Append collsion box list to the display object list.

	  @param objList  The display object list.
	                  It will send to the Renderer.

	  @sa AddCollisionBoxShape()
	*/
#ifdef SSU_DEBUG_DISPLAY_COLLISION_BOX
	void AppendCollisionBox(std::vector<ObjectPtr>& objList) {
	  objList.insert(objList.end(), collisionBoxList.begin(), collisionBoxList.end());
	}
#else
	void AppendCollisionBox(std::vector<ObjectPtr>&) {}
#endif // SSU_DEBUG_DISPLAY_COLLISION_BOX

	/** Set the mouse button press information.

	  @param mx  The mouse cursor position on the X axis.
	  @param my  The mouse cursor position on the Y axis.
	*/
	void PressMouseButton(int mx, int my) {
	  if (debug) {
		mouseX = mx;
		mouseY = my;
		dragging = true;
	  }
	}

	/** Set the mouse buttion release information.
	*/
	void ReleaseMouseButton() {
	  if (debug) {
		dragging = false;
	  }
	}

	/** Set the mouse buttion move information.

	  @param mx  The mouse cursor position on the X axis.
	  @param my  The mouse cursor position on the Y axis.
	*/
	void MoveMouse(int mx, int my) {
	  if (debug) {
		if (dragging) {
		  const float x = static_cast<float>(mouseX - mx) * 0.005f;
		  const float y = static_cast<float>(mouseY - my) * 0.005f;
		  const Vector3F leftVector = camera.eyeVector.Cross(camera.upVector).Normalize();
		  camera.eyeVector = (Quaternion(camera.upVector, x) * Quaternion(leftVector, y)).Apply(camera.eyeVector).Normalize();
		  camera.upVector = Quaternion(leftVector, y).Apply(camera.upVector).Normalize();
		}
		mouseX = mx;
		mouseY = my;
	  }
	}

	/** Move the debug camera position.

	  @param inputList  The key information list for moving.
	                    It should have 4 elements.

	  @sa DirectionKey					 
	*/
	void MoveCamera(const bool* inputList) {
	  if (debug) {
		if (inputList[DIRECTIONKEY_UP]) {
		  camera.position += camera.eyeVector * 0.25f;
		};
		if (inputList[DIRECTIONKEY_DOWN]) {
		  camera.position -= camera.eyeVector * 0.25f;
		}
		if (inputList[DIRECTIONKEY_LEFT]) {
		  camera.position -= camera.eyeVector.Cross(camera.upVector) * 0.25f;
		}
		if (inputList[DIRECTIONKEY_RIGHT]) {
		  camera.position += camera.eyeVector.Cross(camera.upVector) * 0.25f;
		}
	  }
	}

	/** Update the state of all debug objects.

	  @param r          The renderer object.
	  @param deltaTime  The time from previous frame(unit:sec).
	*/
	bool Updata(Renderer& r, float deltaTime) {
	  if (debug) {
		r.Update(deltaTime, camera.position, camera.eyeVector, camera.upVector);
		if (debugObj[0]) {
		  Object& o = *debugObj[0];
		  static float rot = 90, accel = 0.0f;
		  rot += accel;
		  if (rot >= 360) {
			rot -= 360;
		  } else if (rot < 0) {
			rot += 360;
		  }
		  static int target = 2;
		  if (target == 0) {
			o.SetRotation(degreeToRadian<float>(rot), degreeToRadian<float>(0), degreeToRadian<float>(0));
		  } else if (target == 1) {
			o.SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(rot), degreeToRadian<float>(0));
		  } else {
			o.SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(0), degreeToRadian<float>(rot));
		  }
		}
		if (debugObj[1]) {
		  Object& o = *debugObj[1];
		  static float roughness = 0, metallic = 0;
		  o.SetRoughness(roughness);
		  o.SetMetallic(metallic);
		}
	  }
	  return debug;
	}

	bool debug;
	bool dragging;
	int mouseX;
	int mouseY;
	Camera camera;
	std::array<ObjectPtr, 3> debugObj;

#ifdef SSU_DEBUG_DISPLAY_COLLISION_BOX
	std::vector<ObjectPtr>  collisionBoxList;
#endif // SSU_DEBUG_DISPLAY_COLLISION_BOX

#ifdef SHOW_DEBUG_SENSOR_OBJECT
	ObjectPtr debugSensorObj;
#endif // SHOW_DEBUG_SENSOR_OBJECT

#else
	void Clear() {}
	void SetDebugObj(size_t, const ObjectPtr&) {}
	void AddCollisionBoxShape(Renderer&, const Vector3F&, float, float, float, const Vector3F&) {}
	void AppendCollisionBox(std::vector<ObjectPtr>&) {}
	void PressMouseButton(int, int) {}
	void ReleaseMouseButton() {}
	void MoveMouse(int, int) {}
	void MoveCamera(const bool*) {}
	bool Updata(Renderer&, float) { return false; }
#endif // NDEBUG
  };

  static const Region region = { Position3F(-100, 200, -100), Position3F(100, 3800, 100) };
  static const float unitRegionSize = 100.0f;
  static const size_t maxObject = 1000;
  static const float countDownTimerInitialTime = 3.5f;
  static const float countDownTimerStartTime = 3.0f;
  static const float countDownTimerTrailingTime = -1.0f;
  static const int goalHeight = 50;
  static const int unitObstructsSize = 100;
  static const int minObstructHeight = goalHeight + 100;
  static const int offsetTopObstructs = -400;

  /** De Boor algorithm for B-Spline.

    @param k       The counter of the recursion. At first, it shoud equal to 'degree'.
	@param degree  The degree of B-Spline.
	@param i       The interval of line. At first, it should be 'floor(x)'.
	@param x       The position of line. It should be 1 to the size of 'points'.
	@param points  The vector of the control point. It must have a size greater than 'degree'.

	@param The point in the current recursion.

	@note This is the internal function. Please call DeBoor() instead of this.
  */
  Vector3F DeBoorI(int k, int degree, int i, float x, const std::vector<Vector3F>& points) {
	if (k == 0) {
	  return points[std::max(0, std::min<int>(i, points.size() - 1))];
	}
	const float alpha = (x - static_cast<float>(i)) / static_cast<float>(degree + 1 - k);
	const Vector3F a = DeBoorI(k - 1, degree, i - 1, x, points);
	const Vector3F b = DeBoorI(k - 1, degree, i, x, points);
	return a * (1.0f - alpha) + b * alpha;
  }

  /** De Boor algorithm for B-Spline.

    @param degree  The degree of B-Spline.
    @param x       The position of line. It should be 1 to the size of 'points'.
    @param points  The vector of the control point. It must have a size greater than 'degree'.

    @param The point in the current recursion.
  */
  Vector3F DeBoor(int degree, float x, const std::vector<Vector3F>& points) {
	const int i = static_cast<int>(x);
	return DeBoorI(degree, degree, i, x, points);
  }

  /** Create the degree 3 B-Spline curve.

    @param points  The vector of the control point. It must have a size greater than 3.
	@param numOfSegments  The division nunmber of the line.

	@return B-Spline curve. it will contain the 'numOfSegments' elements.
  */
  std::vector<Position3F> CreateBSpline(const std::vector<Vector3F>& points, int numOfSegments) {
	std::vector<Position3F> v;
	v.reserve(numOfSegments);
	const float n = static_cast<float>(points.size() + 1);
	for (int i = 0; i < numOfSegments - 1; ++i) {
	  const float ratio = static_cast<float>(i) / static_cast<float>(numOfSegments - 1);
	  const float x = ratio * n + 1;
	  const Vector3F pos = DeBoor(3, x, points);
	  v.emplace_back(pos.x, pos.y, pos.z);
	}
	v.push_back(points.back().ToPosition3F());
	return v;
  }

  /** Create the control points.

    @param start   The first control point.
	@param goal    The last control point.
	@param count   The number of the control point with 'start' and 'goal'.
	@param random  The random number generator.

	@return B-Spline curve. It will contain the 'numOfSegments' elements.
  */
  template<typename R>
  std::vector<Vector3F> CreateControlPoints(const Position3F& start, const Position3F& goal, int count, R& random) {
	count = std::max(4, count);
	std::vector<Vector3F> v;
	v.reserve(count);
	v.emplace_back(start.x, start.y, start.z);
	const Vector3F distance = goal - start;
	Position3F center(0, 0, 0);
	std::uniform_real_distribution<float> tgen(0.0f, 360.0f);
	float range = 25.0f;
	for (int i = 1; i < count - 1; ++i) {
	  const Position3F p = start + distance * static_cast<float>(i) / static_cast<float>(count - 1);
	  const float theta = degreeToRadian(tgen(random));
	  float r = std::uniform_real_distribution<float>(0.0f, range)(random);
	  const float tx = center.x + std::cos(theta) * r;
	  const float tz = center.z + std::sin(theta) * r;
	  v.emplace_back(p.x + tx, p.y, p.z + tz);
	  center = Position3F(tx, 0, tz) * (r - range) / r;
	  if (i < count / 2) {
		range = 100.0f;
	  } else {
		range = 50.0f;
	  }
	}
	v.emplace_back(goal.x, goal.y, goal.z);
	return v;
  }

  /** Create the model route.

    @param start   The start point of the route.
	@param goal    The end(goal) point of the route.
	@param random  The random number generator.

	@return The model route.

	This function creates the route based on B-Spline.
	At first, generate some control points at regular intervals.
	Then, generate more points at equal intervals on the curve.
  */
  template<typename R>
  std::vector<Position3F> CreateModelRoute(const Position3F& start, const Position3F& goal, R& random) {
	const int length = std::abs(static_cast<int>(goal.y - start.y));
	const std::vector<Vector3F> controlPoints = CreateControlPoints(start, goal, (length + 499) / 500 + 2, random);
	return CreateBSpline(controlPoints, (length + 99) / 100);
  }

  /** Get the number of digits.

    @param n  The number. Ignore after the decimal point.

	@return The number of digiets of the integral part.
  */
  size_t GetNumberOfDigits(float n) {
	if (n != 0.0f && !std::isnormal(n)) {
	  return 1;
	}
	int64_t i = static_cast<int64_t>(n);
	size_t num = 1;
	while (i >= 10) {
	  ++num;
	  i /= 10;
	}
	return num;
  }

  /** Convert the digits to string.

    @param n        The digits.
	@param num      The maximum number of digits.
	@param padding  If true, do padding with zero. Otherwize not padding.

	@return The string converted from 'n'.
  */
  std::string DigitsToString(float n, size_t num, bool padding) {
	std::string  s;
	const size_t nod = GetNumberOfDigits(n);
	s.reserve(nod);
	int32_t nn = static_cast<int32_t>(n);
	if (n != 0.0f && !std::isnormal(n)) {
	  nn = 0;
	}
	for (size_t i = 0; i < nod; ++i) {
	  s.push_back(static_cast<char>(nn % 10) + '0');
	  nn /= 10;
	};
	if (padding && nod < num) {
	  const char pad = padding ? '0' : ' ';
	  s.append(num - nod, pad);
	}
	std::reverse(s.begin(), s.end());
	return s;
  }

  /** Control the main game play.

    Player start to fall down from the range of 4000m from 2000m.
	When player reaches 100m, it is success if the moving vector is facing
	within the range of target. Otherwise fail.
  */
  class MainGameScene : public Scene {
  public:
	MainGameScene()
	  : initialized(false)
	  , pPartitioner()
	  , random(static_cast<uint32_t>(time(nullptr)))
	  , playerMovement(0, 0, 0)
	  , playerRotation(0, 0, 0)
	  , countDownTimer(countDownTimerInitialTime)
	  , stopWatch(0)
	  , updateFunc(&MainGameScene::DoUpdate)
	  , debugData()
	{
	  directionKeyDownList.fill(false);
	}
	virtual ~MainGameScene() {}

	/** Load the game objects.

	  @param engine  The engine object.
	*/
	virtual bool Load(Engine& engine) {
	  if (initialized) {
		status = STATUSCODE_RUNNABLE;
		return true;
	  }
	  directionKeyDownList.fill(false);
	  Renderer& renderer = engine.GetRenderer();
	  pPartitioner.reset(new SpacePartitioner(region.min, region.max, unitRegionSize, maxObject));

	  const int level = std::min(GetMaximumLevel(), engine.GetCommonData<CommonData>()->level);
	  const int courseNo = std::min(GetMaximumCourseNo(level), engine.GetCommonData<CommonData>()->courseNo);
	  const CourseInfo& courseInfo = GetCourseInfo(level, courseNo);
	  random.seed(courseInfo.seed);

	  // Set a scene lighting information.
	  {
		const TimeOfScene tos = [courseInfo]() {
		  if (courseInfo.hoursOfDay >= 7 && courseInfo.hoursOfDay < 16) {
			return TimeOfScene_Noon;
		  } else if (courseInfo.hoursOfDay >= 16 && courseInfo.hoursOfDay < 19) {
			return TimeOfScene_Sunset;
		  } else {
			return TimeOfScene_Night;
		  }
		}();
		renderer.SetTimeOfScene(tos);
		renderer.DoesDrawSkybox(false);

		const Vector3F shadowDir = GetSunRayDirection(tos);
		const int level = std::min(GetMaximumLevel(), engine.GetCommonData<CommonData>()->level);
		const int courseNo = std::min(GetMaximumCourseNo(level), engine.GetCommonData<CommonData>()->courseNo);
		const float radius = (static_cast<float>(GetCourseInfo(level, courseNo).startHeight) + 10.0f) * 0.5f;
		renderer.SetShadowLight(Position3F(0, radius, 0) - shadowDir * radius, shadowDir, 10, radius * 2.0f, Vector2F(0.5f, 1.0f / 3.0f));
	  }

	  // The player character.
	  {
		auto obj = renderer.CreateObject("ChickenEgg", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		Object& o = *obj;
		o.SetAnimation(renderer.GetAnimation("Dive"));
		const Vector3F trans(5, static_cast<GLfloat>(courseInfo.startHeight), 4.5f);
		o.SetTranslation(trans);
		playerRotation = Vector3F(degreeToRadian<float>(0), degreeToRadian<float>(180), degreeToRadian<float>(0));
		o.SetRotation(playerRotation.x, playerRotation.y, playerRotation.z);
		//o.SetScale(Vector3F(5, 5, 5));
		Collision::RigidBodyPtr p(new Collision::SphereShape(trans.ToPosition3F(), 3.0f, 0.1f));
		//p->thrust = Vector3F(0, 9.8f, 0);
		pPartitioner->Insert(obj, p, Vector3F(0, 0, 0));
		rigidCamera = p;
		objPlayer = obj;
	  }

	  // The target(as known as Flying-pan).
	  {
		auto obj = renderer.CreateObject("FlyingPan", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		Object& o = *obj;
		o.SetScale(Vector3F(10, 10, 10));

		const float theta = degreeToRadian<float>(RandomFloat(360));
		const float distance = RandomFloat(100);
		const float tx = std::cos(theta) * (distance * 2.0f + 10.0f);
		const float tz = std::sin(theta) * (distance + 20.0f);
		o.SetTranslation(Vector3F(tx, 40, tz));
		pPartitioner->Insert(obj);
		objFlyingPan = obj;
	  }

	  {
		auto obj = renderer.CreateObject("TargetCursor", Material(Color4B(240, 2, 1, 255), 0, 0), "emission", ShadowCapability::Disable);
		Object& o = *obj;
		o.SetAnimation(renderer.GetAnimation("Rotation"));
		o.SetScale(Vector3F(20, 20, 20));
		o.SetTranslation(objFlyingPan->Position() - Position3F(0, -10, 0));
		pPartitioner->Insert(obj);
	  }

	  {
		static const size_t posListSize = 5;
		const std::vector<Position3F> modelRoute = CreateModelRoute(Position3F(5, static_cast<float>(courseInfo.startHeight), 4.5f), objFlyingPan->Position() + Vector3F(0, static_cast<float>(goalHeight), 0), random);
		const auto end = modelRoute.end() - 2;
		const float step = static_cast<float>(unitObstructsSize) * std::max(1.0f, (4.0f - static_cast<float>(courseInfo.difficulty) * 0.5f));
		const int density = std::min<int>(posListSize, courseInfo.density);
		for (float height = static_cast<float>(courseInfo.startHeight + offsetTopObstructs); height > static_cast<float>(minObstructHeight); height -= step) {
		  auto itr = std::upper_bound(modelRoute.begin(), modelRoute.end(), height,
			[](float h, const Position3F& p) { return h > p.y; }
		  );
		  if (itr >= end) {
			continue;
		  }
		  const Position3F& e = *itr;
		  Vector3F axis(*(itr + 1) - e);
		  axis.y = 0;
		  if (axis.LengthSq() < 0.0001f) {
			axis.x = axis.z = 1.0f;
		  }
		  axis.Normalize();
		  axis *= RandomFloat(10) + 70.0f;
		  const Vector3F posList[posListSize] = {
			Vector3F(e.x, e.y, e.z),
			Vector3F(e.x - axis.x, e.y, e.z - axis.z),
			Vector3F(e.x + axis.z, e.y, e.z - axis.x),
			Vector3F(e.x - axis.z, e.y, e.z + axis.x),
			Vector3F(e.x + axis.z, e.y, e.z + axis.x)
		  };
		  for (int j = 0; j < density; ++j) {
			if (random() % 3 < 2) {
			  auto obj = renderer.CreateObject("rock_s", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
			  Object& o = *obj;
			  const Vector3F trans = posList[j];
			  o.SetTranslation(trans);
			  const float scale = 12;
			  o.SetScale(Vector3F(scale, scale, scale));
			  const float rx = degreeToRadian<float>(RandomFloat(30));
			  const float ry = degreeToRadian<float>(RandomFloat(360));
			  const float rz = degreeToRadian<float>(RandomFloat(30));
			  o.SetRotation(rx, ry + degreeToRadian(45.0f), rz);
			  const Vector3F collisionScale(Vector3F(0.625f, 1.25f, 0.625f) * scale);
			  Collision::RigidBodyPtr p(new Collision::BoxShape(trans.ToPosition3F(), o.RotTrans().rot, collisionScale, scale * scale * scale * (5 * 6 * 5 / 3.0f)));
			  o.SetRotation(rx, ry, rz);
			  p->thrust = Vector3F(0, 9.8f, 0);
			  pPartitioner->Insert(obj, p);
			  debugData.AddCollisionBoxShape(renderer, trans, rx, ry + degreeToRadian(45.0f), rz, collisionScale * 2.0f);
			} else {
			  auto obj = renderer.CreateObject("FlyingRock", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
			  Object& o = *obj;
			  const Vector3F trans = posList[j];
			  o.SetTranslation(trans);
			  const float scale = 6;
			  o.SetScale(Vector3F(scale, scale, scale));
			  const float rx = degreeToRadian<float>(RandomFloat(30));
			  const float ry = degreeToRadian<float>(RandomFloat(360));
			  const float rz = degreeToRadian<float>(RandomFloat(30));
			  o.SetRotation(rx, ry, rz);
			  const Vector3F collisionScale(Vector3F(2.0f, 2.5f, 2.0f) * scale);
			  Collision::RigidBodyPtr p(new Collision::BoxShape(trans.ToPosition3F(), o.RotTrans().rot, collisionScale, scale * scale * scale * (5 * 6 * 5 / 3.0f)));
			  p->thrust = Vector3F(0, 9.8f, 0);
			  pPartitioner->Insert(obj, p);
			  debugData.AddCollisionBoxShape(renderer, trans, rx, ry, rz, collisionScale * 2.0f);
			}
		  }
		}
	  }
	  for (float height = static_cast<float>(courseInfo.startHeight + offsetTopObstructs); height > static_cast<float>(minObstructHeight); height -= static_cast<float>(unitObstructsSize) * 5) {
		auto obj = renderer.CreateObject("block1", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		Object& o = *obj;
		const float h = RandomFloat(unitObstructsSize * 2) + static_cast<float>(unitObstructsSize) * 0.5f;
		const float theta = degreeToRadian<float>(RandomFloat(360));
		const float distance = RandomFloat(150);
		const Vector3F trans(std::cos(theta) * distance, height + h, std::sin(theta) * distance);
		o.SetTranslation(trans);
		const float scale = 5.0f;
		o.SetScale(Vector3F(scale, scale, scale));
		const float rx = degreeToRadian<float>(RandomFloat(90) - 45.0f);
		const float ry = degreeToRadian<float>(RandomFloat(360));
		o.SetRotation(rx, ry, 0);
		const Vector3F collisionScale(Vector3F(4.8f, 0.375f, 3.8f) * scale);
		Collision::RigidBodyPtr p(new Collision::BoxShape(trans.ToPosition3F(), o.RotTrans().rot, collisionScale, 10 * 1 * 8));
		p->thrust = Vector3F(0, 9.8f, 0);
		pPartitioner->Insert(obj, p);
		debugData.AddCollisionBoxShape(renderer, trans, rx, ry, 0, collisionScale * 2.0f);
	  }
#if 0
	  {
		auto obj = renderer.CreateObject("Accelerator", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		Object& o = *obj;
		o.SetScale(Vector3F(5, 5, 5));
		o.SetTranslation(Vector3F(-15, 120, 0));
		o.SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(0), degreeToRadian<float>(0));
		pPartitioner->Insert(obj);
	  }
#endif
	  if (auto pBuf = FileSystem::LoadFile("Meshes/CoastTownSpace.msh")) {
		const auto result = Mesh::ImportGeometry(*pBuf);
		if (result.result == Mesh::Result::success && !result.geometryList.empty()) {
		  const Mesh::Geometry& m = result.geometryList[0];
		  static const char* const meshNameList[] = {
			"CityBlock00",
			"CityBlock01",
			"CityBlock02",
			"CityBlock03",
			"CityBlock04",
			"CityBlock05",
			"CityBlock06",
			"CityBlock07",
		  };
		  static const Color4B colorList[] = {
			Color4B(200, 200, 200, 255),
			Color4B(255, 200, 200, 255),
			Color4B(200, 255, 200, 255),
			Color4B(200, 200, 255, 255),
			Color4B(255, 255, 200, 255),
		  };
		  int i = 0;
		  for (const auto& e : m.vertexList) {
			auto obj = renderer.CreateObject(
			  meshNameList[i % (sizeof(meshNameList) / sizeof(meshNameList[0]))],
			  Material(colorList[i % (sizeof(colorList) / sizeof(colorList[0]))], 0, 0),
			  "default",
			  ShadowCapability::Disable
			);
			obj->SetScale(Vector3F(4, 4, 4));
			obj->SetTranslation(Vector3F(e.position.x * 12.0f, e.position.y * 12.0f, e.position.z * 12.0f));
			const float ry = std::asin(e.tangent.z / e.tangent.x);
			obj->SetRotation(degreeToRadian<float>(0), ry, degreeToRadian<float>(0));
			pPartitioner->Insert(obj);
			++i;
		  }
		}
	  }
	  {
		auto obj = renderer.CreateObject("LandScape.Coast", Material(Color4B(255, 255, 255, 255), 0, 0), "solidmodel", ShadowCapability::Disable);
		obj->SetScale(Vector3F(12, 12, 12));
		Collision::RigidBodyPtr p(new Collision::PlaneShape(Position3F(0, 0, 0), Vector3F(0, 1, 0), 1000 * 1000 * 1000));
		p->thrust = Vector3F(0, 9.8f, 0);
		pPartitioner->Insert(obj, p);
	  }
	  {
		auto obj = renderer.CreateObject("LandScape.Coast.Levee", Material(Color4B(255, 255, 255, 255), 0, 0), "default", ShadowCapability::Disable);
		obj->SetScale(Vector3F(12, 12, 12));
		pPartitioner->Insert(obj);
	  }
	  {
		auto obj = renderer.CreateObject("LandScape.Coast.Flora", Material(Color4B(200, 200, 200, 255), 0, 0), "defaultWithAlpha", ShadowCapability::Disable);
		obj->SetScale(Vector3F(12, 12, 12));
		pPartitioner->Insert(obj);
	  }
	  {
		auto obj = renderer.CreateObject("LandScape.Coast.Ships", Material(Color4B(200, 200, 200, 255), 0, 0), "defaultWithAlpha", ShadowCapability::Disable);
		obj->SetScale(Vector3F(12, 12, 12));
		pPartitioner->Insert(obj);
	  }

	  // Add cloud.
	  {
		for (int i = 0; i < 5; ++i) {
		  const int cloudCount = ((i * i / 2 + 1) * courseInfo.cloudage) / courseInfo.maxCloudage;
		  const int heightMax = 2600 - i * 400;
		  const int heightMin = heightMax - 400;
		  const int radiusMax = (i * i + 4) * 25;
		  const int radiusMax2 = radiusMax * radiusMax;
		  for (int j = 0; j < cloudCount; ++j) {
			static const char* const idList[] = { "cloud0", "cloud1", "cloud2", "cloud3" };
			const int index = boost::random::uniform_int_distribution<>(0, 3)(random);
			const float y = static_cast<float>(boost::random::uniform_int_distribution<>(heightMin, heightMax)(random));
			float r = static_cast<float>(boost::random::uniform_int_distribution<>(10, radiusMax)(random));
			//r = radiusMax - r * r / radiusMax2;
			const float a0 = static_cast<float>(boost::random::uniform_int_distribution<>(0, 359)(random));
			const float a1 = static_cast<float>(boost::random::uniform_int_distribution<>(30, 60)(random));
			const float scale = boost::random::uniform_int_distribution<>(10, 30)(random) *  0.1f;

			static const GLubyte alphaList[] = { 64, 64, 96, 96, 96, 96, 96, 128, 128, 192 };
			const int alphaIndex = boost::random::uniform_int_distribution<>(0, sizeof(alphaList)/sizeof(alphaList[0]) - 1)(random);

			static const GLubyte colorList[] = { 255, 224, 192 };
			const int colorIndex = boost::random::uniform_int_distribution<>(0, sizeof(colorList) / sizeof(colorList[0]) - 1)(random);
			const GLubyte color = colorList[colorIndex];

			auto obj = renderer.CreateObject(idList[index], Material(Color4B(color, color, color, alphaList[alphaIndex]), 0, 0), "cloud", ShadowCapability::Disable);
			Object& o = *obj;
			const Quaternion rot0(Vector3F(0, 1, 0), degreeToRadian<float>(a0));
			const Quaternion rot1(Vector3F(0, 1, 0), degreeToRadian<float>(a0));
			const Vector3F trans = rot0.Apply(Vector3F(0, y, r));
			o.SetTranslation(trans);
			o.SetRotation(rot1);
			o.SetScale(Vector3F(scale, scale, scale));
			pPartitioner->Insert(obj);
		  }
		}
	  }

	  AudioInterface& audio = engine.GetAudio();
	  audio.LoadSE("bound", "Audio/bound.wav");
	  audio.LoadSE("success", "Audio/success.wav");
	  audio.LoadSE("failure", "Audio/miss.wav");
	  audio.PlayBGM("Audio/dive.mp3", 1.0f);
	  status = STATUSCODE_RUNNABLE;
	  initialized = true;
	  return true;
	}

	/** Unload all game object.

	  @param engine  The engine object.
	*/
	virtual bool Unload(Engine& engine) {
	  debugData.Clear();
	  rigidCamera.reset();
	  objPlayer.reset();
	  objFlyingPan.reset();
	  pPartitioner->Clear();
	  status = STATUSCODE_STOPPED;
	  initialized = false;
	  return true;
	}

	/** Process the window events.

	  @param engine  The engine object.
	  @param e       The window event.
	*/
	virtual void ProcessWindowEvent(Engine&, const Event& e) {
	  switch (e.Type) {
	  case Event::EVENT_MOUSE_BUTTON_PRESSED:
		if (e.MouseButton.Button == MOUSEBUTTON_LEFT) {
		  debugData.PressMouseButton(e.MouseButton.X, e.MouseButton.Y);
		}
		break;
	  case Event::EVENT_MOUSE_BUTTON_RELEASED:
		if (e.MouseButton.Button == MOUSEBUTTON_LEFT) {
		  debugData.ReleaseMouseButton();
		}
		break;
	  case Event::EVENT_MOUSE_ENTERED:
		break;
	  case Event::EVENT_MOUSE_LEFT:
		debugData.ReleaseMouseButton();
		break;
	  case Event::EVENT_MOUSE_MOVED: {
		debugData.MoveMouse(e.MouseMove.X, e.MouseMove.Y);
		break;
	  }
	  case Event::EVENT_KEY_PRESSED:
		switch (e.Key.Code) {
		case KEY_W:
		  directionKeyDownList[DIRECTIONKEY_UP] = true;
		  break;
		case KEY_S:
		  directionKeyDownList[DIRECTIONKEY_DOWN] = true;
		  break;
		case KEY_A:
		  directionKeyDownList[DIRECTIONKEY_LEFT] = true;
		  break;
		case KEY_D:
		  directionKeyDownList[DIRECTIONKEY_RIGHT] = true;
		  break;
		default:
		  /* DO NOTING */
		  break;
		}
		break;
	  case Event::EVENT_KEY_RELEASED:
		switch (e.Key.Code) {
		case KEY_W:
		  directionKeyDownList[DIRECTIONKEY_UP] = false;
		  break;
		case KEY_S:
		  directionKeyDownList[DIRECTIONKEY_DOWN] = false;
		  break;
		case KEY_A:
		  directionKeyDownList[DIRECTIONKEY_LEFT] = false;
		  break;
		case KEY_D:
		  directionKeyDownList[DIRECTIONKEY_RIGHT] = false;
		  break;
		default:
		  /* DO NOTING */
		  break;
		}
		break;
      case Event::EVENT_TILT: {
		static const float offsetY = 0.2f;
		static const float margin = 0.05f;
		static const float scale = 20.f;
        const float tz = e.Tilt.Z;
        if (std::abs(tz) <= margin) {
          playerMovement.x = 0.0f;
        } else if (tz < 0.0f) {
          playerMovement.x = (tz + margin) * scale;
        } else {
          playerMovement.x = (tz - margin) * scale;
        }
		const float ty = e.Tilt.Y + offsetY;
        if (std::abs(ty) < margin) {
          playerMovement.z = 0.0f;
        } else if (ty < 0.0f) {
          playerMovement.z = -(ty + margin) * scale;
        } else {
          playerMovement.z = -(ty - margin) * scale;
        }
        break;
      }
	  default:
		break;
	  }
	}

	/** Update the state of all game objects.

	  @param engine     The engine object.
	  @param deltaTime  The time from previous frame(unit:sec).
	*/
	virtual int Update(Engine& engine, float deltaTime) {
	  return (this->*updateFunc)(engine, deltaTime);
	}

	/** update game play.

	  @param engine  The engine object.
	  @param e       The window event.

	  Transition to DoFadeOut() when the player character reached the target height.
	  This is the update function called from Update().
	*/
	int DoUpdate(Engine& engine, float deltaTime) {
#if 1
	  debugData.MoveCamera(&directionKeyDownList[0]);
	  //debugCamera.Update(deltaTime, fusedOrientation);
	  if (countDownTimer >= countDownTimerTrailingTime) {
		countDownTimer -= deltaTime;
	  }
	  if (countDownTimer <= 0.0f) {
		if (objPlayer->Position().y >= goalHeight) {
		  stopWatch += deltaTime;
		}
#ifndef __ANDROID__
		if (directionKeyDownList[DIRECTIONKEY_UP]) {
		  rigidCamera->thrust.z = std::min(15.0f, rigidCamera->thrust.z - 1.0f);
		} else if (directionKeyDownList[DIRECTIONKEY_DOWN]) {
		  rigidCamera->thrust.z = std::max(-15.0f, rigidCamera->thrust.z + 1.0f);
		} else {
		  rigidCamera->thrust.z *= 0.9f;
		  if (std::abs(rigidCamera->thrust.z) < 0.1) {
			rigidCamera->thrust.z = 0.0f;
		  }
		}
		if (directionKeyDownList[DIRECTIONKEY_LEFT]) {
		  rigidCamera->thrust.x = std::min(15.0f, rigidCamera->thrust.x - 1.0f);
		} else if (directionKeyDownList[DIRECTIONKEY_RIGHT]) {
		  rigidCamera->thrust.x = std::max(-15.0f, rigidCamera->thrust.x + 1.0f);
		} else {
		  rigidCamera->thrust.x *= 0.9f;
		  if (std::abs(rigidCamera->thrust.x) < 0.1) {
			rigidCamera->thrust.x = 0.0f;
		  }
		}
		rigidCamera->thrust.y = 0.0f;
		if (rigidCamera->accel.y < 0.0f) {
		  rigidCamera->thrust.y = std::min(-rigidCamera->accel.y, std::max(0.0f, rigidCamera->thrust.Length() * 0.5f));
		}
		playerRotation.x = std::min(0.5f, std::max(-0.5f, rigidCamera->thrust.z * -0.05f));
		playerRotation.z = std::min(0.5f, std::max(-0.5f, rigidCamera->thrust.x * -0.05f));
		objPlayer->SetRotation(playerRotation.x, playerRotation.y, playerRotation.z);
#else
		rigidCamera->thrust = playerMovement;
		rigidCamera->thrust.y = 0.0f;
		if (rigidCamera->accel.y < 0.0f) {
		  rigidCamera->thrust.y = std::min(-rigidCamera->accel.y, std::max(0.0f, rigidCamera->thrust.Length() * 0.5f));
		}
		playerRotation.x = std::min(0.5f, std::max(-0.5f, playerMovement.z * -0.05f));
		playerRotation.z = std::min(0.5f, std::max(-0.5f, playerMovement.x * -0.05f));
		objPlayer->SetRotation(playerRotation.x, playerRotation.y, playerRotation.z);
#endif // __ANDROID__
	  }
	  pPartitioner->Update(deltaTime);
	  if (rigidCamera && rigidCamera->hasLatestCollision && rigidCamera->accel.LengthSq() > (2.0f * 2.0f)) {
		engine.GetAudio().PlaySE("bound", 1.0f);
	  }
#else
	  const Position3F prevCamPos = debugCamera.Position();
	  debugCamera.Update(deltaTime, fusedOrientation);
	  Vector3F camVec = debugCamera.Position() - prevCamPos;
	  if (rigidCamera) {
		if (camVec.LengthSq() == 0.0f) {
		  Vector3F v = rigidCamera->accel;
		  v.y = 0;
		  if (v.LengthSq() < 0.1f) {
			v = Vector3F::Unit();
		  } else {
			v.Normalize();
			v *= 0.1f;
		  }
		  rigidCamera->accel -= v;
		} else {
		  rigidCamera->accel = camVec * 10.0f;
		}
		if (rigidCamera->Position().y < 5.0f && debugCamera.Direction() == TouchSwipeCamera::MoveDir_Back) {
		  debugCamera.Direction(TouchSwipeCamera::MoveDir_Newtral);
		  rigidCamera->Position(Position3F(0, 4000, 0));
		}
	  }
	  pPartitioner->Update(engine.deltaTime);
	  if (rigidCamera) {
		Position3F pos = rigidCamera->Position();
		pos.y += 10.0f;
		debugCamera.Position(pos);
	  }
#endif
	  Renderer& renderer = engine.GetRenderer();
	  if (!debugData.Updata(renderer, deltaTime)) {
		renderer.Update(deltaTime, rigidCamera->Position() + Vector3F(0, 20, -2), Vector3F(0, -1, 0), Vector3F(0, 0, -1));
		//			for (auto&& e : engine.obj) {
		//				e.Update(engine.deltaTime);
		//				while (e.GetCurrentTime() >= 1.0f) {
		//					e.SetCurrentTime(e.GetCurrentTime() - 1.0f);
		//				}
		//			}
	  }

#if 0
	  static float roughness = 1.0f;
	  static float step = 0.005;
	  obj[1].SetRoughness(roughness);
	  roughness += step;
	  if (roughness >= 1.0) {
		step = -0.005f;
	  } else if (roughness <= 0.0) {
		step = 0.005f;
	  }
#endif
	  if (objPlayer->Position().y <= goalHeight) {
		AudioInterface& audio = engine.GetAudio();
		audio.StopBGM(5.0f);
		renderer.FadeOut(Color4B(0, 0, 0, 0), 1.0f);
		updateFunc = &MainGameScene::DoFadeOut;
	  }
	  return SCENEID_CONTINUE;
	}

	/** Do fade out.

	  @param engine     The engine object.
	  @param deltaTime  The time from previous frame(unit:sec).

	  Transition to the scene of the success/failure event when the fadeout finished.
	  This is the update function called from Update().
	*/
	int DoFadeOut(Engine& engine, float deltaTime) {
	  Renderer& renderer = engine.GetRenderer();
	  if (!debugData.Updata(renderer, deltaTime)) {
		renderer.Update(deltaTime, rigidCamera->Position() + Vector3F(0, 20, -2), Vector3F(0, -1, 0), Vector3F(0, 0, -1));
	  }
	  if (renderer.GetCurrentFilterMode() == Renderer::FILTERMODE_NONE) {
		engine.GetCommonData<CommonData>()->currentTime = static_cast<int64_t>(stopWatch * 1000.0f);
		renderer.FadeIn(1.0f);
		const float scale = objFlyingPan->Scale().x;
		const Vector3F v = objPlayer->Position() - objFlyingPan->Position();
		const float distance = std::sqrt(v.x * v.x + v.z * v.z);
		if (distance <= 1.25f * scale) {
		  engine.GetAudio().PlaySE("success", 1.0f);
		  return SCENEID_SUCCESS;
		}
		engine.GetAudio().PlaySE("failure", 1.0f);
		return SCENEID_FAILURE;
	  }
	  return SCENEID_CONTINUE;
	}

	/** Draw all game objects.

	  @param engine  The engine object.
	*/
	virtual void Draw(Engine& engine) {
	  std::vector<ObjectPtr> objList;
	  objList.reserve(10 * 10);
#if 0
	  float posY = debugCamera.Position().y;
	  if (debugCamera.EyeVector().y >= 0.0f) {
		posY = std::max(region.min.y, posY - unitRegionSize * 2);
		for (int i = 0; i < 6; ++i) {
		  const Cell& cell = pPartitioner->GetCell(posY);
		  for (auto& e : cell.objects) {
			objList.push_back(e.object);
		  }
		  posY += unitRegionSize;
		  if (posY >= region.max.y) {
			break;
		  }
		}
	  } else {
		posY = std::min(region.max.y, posY + unitRegionSize * 2);
		for (int i = 0; i < 6; ++i) {
		  const Cell& cell = pPartitioner->GetCell(posY);
		  for (auto& e : cell.objects) {
			objList.push_back(e.object);
		  }
		  posY -= unitRegionSize;
		  if (posY < region.min.y) {
			break;
		  }
		}
	  }

	  const Position3F eyePos = debugCamera.Position();
	  const Vector3F eyeVec = debugCamera.EyeVector();
	  objList.erase(
		std::remove_if(
		  objList.begin(),
		  objList.end(),
		  [eyePos, eyeVec](const ObjectPtr& n) {
		const Vector3F tmp = n->Position() - eyePos;
		return (tmp.LengthSq() > 15.0f * 15.0f) && (eyeVec.Dot(Vector3F(tmp).Normalize()) < 0.0f);
	  }
		  ),
		objList.end()
		);
	  std::sort(
		objList.begin(),
		objList.end(),
		[eyePos](const ObjectPtr& lhs, const ObjectPtr& rhs) {
		const Vector3F v0 = lhs->Position() - eyePos;
		const Vector3F v1 = rhs->Position() - eyePos;
		return v0.LengthSq() < v1.LengthSq();
	  }
	  );
#else
	  for (auto itr = pPartitioner->Begin(); itr != pPartitioner->End(); ++itr) {
		for (auto& e : itr->objects) {
		  objList.push_back(e.object);
		}
	  }
#endif
	  debugData.AppendCollisionBox(objList);

#ifdef SHOW_DEBUG_SENSOR_OBJECT
	  {
		Position3F pos = debugCamera.Position();
		const Vector3F ev = debugCamera.EyeVector();
		const Vector3F uv = debugCamera.UpVector();
		const Vector3F sv = ev.Cross(uv);
		pos += ev * 4.0f;
		pos += -uv * 1.5f;
		pos += sv * 0.9f;
		debugSensorObj->SetTranslation(Vector3F(pos.x, pos.y, pos.z));

		// fusedOrientation has each axis range of x(+pi, -pi), y(+pi/2, -pi/2), z(+pi, -pi).
		const float rx = (fusedOrientation.x + boost::math::constants::pi<float>()) * 0.5f;
		const float ry = -fusedOrientation.y + boost::math::constants::pi<float>() * 0.5f;
		const float rz = (fusedOrientation.z + boost::math::constants::pi<float>()) * 0.5f;
		Matrix4x4 mRot = LookAt(Position3F(0, 0, 0), Position3F(ev.x, ev.y, ev.z), uv);
		mRot = Matrix4x4::RotationX(rx) * Matrix4x4::RotationX(ry) * Matrix4x4::RotationX(rz) * mRot;
		Quaternion q;
		mRot.Decompose(&q, nullptr, nullptr);
		debugSensorObj->SetRotation(q);
		objList.push_back(debugSensorObj);
	  }
#endif // SHOW_DEBUG_SENSOR_OBJECT

	  Renderer& renderer = engine.GetRenderer();
	  if (rigidCamera) {
		static float numberFontScale = 0.8f;
		static float fontScale = 2.0f / 3.0f;
		static float uw = 1.0f / 12.0f / fontScale;
		static float cw = 1.0f / 20.0f;
		static float cw0 = 1.0f / 20.0f;
		static float baseX = 0.75f;

		{
		  const float n = std::abs(rigidCamera->accel.y) * 3.6f;
		  const float nod = static_cast<float>(GetNumberOfDigits(n));
		  const std::string s = DigitsToString(n, 3, false);
		  renderer.AddString(baseX - cw * nod, 0.05f, numberFontScale, Color4B(255, 255, 255, 255), s.c_str(), uw);
		  renderer.AddString(baseX, 0.055f, fontScale, Color4B(255, 255, 255, 255), "Km/h");
		}

		{
		  float y = objPlayer->Position().y;
		  if (auto radius = rigidCamera->GetRadius()) {
			y -= *radius;
		  }
		  const float nod = static_cast<float>(GetNumberOfDigits(y));
		  const std::string s = DigitsToString(y, 4, false);
		  renderer.AddString(baseX - cw * (nod - 1.0f), 0.1f, numberFontScale, Color4B(255, 255, 255, 255), s.c_str(), uw);
		  renderer.AddString(baseX + cw0, 0.105f, fontScale, Color4B(255, 255, 255, 255), "M");
		}

		{
		  const float nod = static_cast<float>(GetNumberOfDigits(stopWatch));
		  const std::string s = DigitsToString(stopWatch, 3, false);
		  renderer.AddString(baseX - cw * (nod + 2.5f), 0.15f, numberFontScale, Color4B(255, 255, 255, 255), s.c_str(), uw);
		  renderer.AddString(baseX - cw * 2.5f, 0.15f, fontScale, Color4B(255, 255, 255, 255), ".");
		}
		{
		  const float pointDecimal = (stopWatch - std::floor(stopWatch)) * 1000.0f;
		  const std::string s = DigitsToString(pointDecimal, 3, true);
		  renderer.AddString(baseX - cw * 2.0f, 0.15f, numberFontScale, Color4B(255, 255, 255, 255), s.c_str(), uw);
		  renderer.AddString(baseX + cw0, 0.155f, fontScale, Color4B(255, 255, 255, 255), "Sec");
		}
		{
		  CommonData& commonData = *engine.GetCommonData<CommonData>();
		  std::string s = DigitsToString(commonData.level + 1, 1, false);
		  s += "-";
		  s += DigitsToString(commonData.courseNo + 1, 1, false);
		  renderer.AddString(0.05f, 0.05f, numberFontScale, Color4B(255, 255, 255, 255), s.c_str(), uw);
		}

		if (countDownTimer > 0.0f) {
		  static const char strReady[] = "READY!";
		  static const char* const strNumber[] = { "1", "2", "3" };
		  renderer.AddString(0.5f - renderer.GetStringWidth(strReady), 0.3f, 2.0f, Color4B(255, 240, 200, 255), strReady);
		  if (countDownTimer < countDownTimerStartTime) {
			const int index = static_cast<int>(countDownTimer);
			float dummy;
			const float scale = 4.0f - std::modf(countDownTimer, &dummy) * 2.0f;
			const float w = renderer.GetStringWidth(strNumber[index]) * 0.5f * scale;
			const float h = renderer.GetStringHeight(strNumber[index]) * 0.5f * scale;
			const GLbyte a = static_cast<GLbyte>(255.0f * (2.0f - scale * 0.5f));
			renderer.AddString(0.5f - w, 0.5f - h, scale, Color4B(255, 255, 255, a), strNumber[index]);
		  }
		} else if (countDownTimer > countDownTimerTrailingTime) {
		  static const char str[] = "GO!";
		  const float scale = 2.0f + (-countDownTimer / -countDownTimerTrailingTime) * 3.0f;
		  const float w = renderer.GetStringWidth(str) * 0.5f * scale;
		  const float h = renderer.GetStringHeight(str) * 0.5f * scale;
		  const GLbyte a = static_cast<GLbyte>(255.0f * (1.0f - (scale - 2.0f) / 3.0f));
		  renderer.AddString(0.5f - w, 0.5f - h, scale, Color4B(255, 200, 155, a), str);
		}
	  }
	  renderer.Render(&objList[0], &objList[0] + objList.size());
	}

  private:
	void InsertObject(
	  const ObjectPtr& obj,
	  const Collision::RigidBodyPtr& c = Collision::RigidBodyPtr(),
	  const Vector3F& offset = Vector3F::Unit()
	  ) {
	  if (pPartitioner) {
		pPartitioner->Insert(obj, c, offset);
	  }
	}
	template<typename T>
	float RandomFloat(T n) { return static_cast<float>(random() % n); }

  private:
	bool initialized;
	std::unique_ptr<SpacePartitioner> pPartitioner;
	boost::random::mt19937 random;
	Collision::RigidBodyPtr rigidCamera;
	ObjectPtr objPlayer;
	ObjectPtr objFlyingPan;
	Vector3F playerMovement;
	Vector3F playerRotation;
	float countDownTimer;
	float stopWatch;
	int(MainGameScene::*updateFunc)(Engine&, float);

	std::array<bool, 4> directionKeyDownList;

	DebugData debugData;
  };

  /** Create MainGameScene object.

	@param engine  The engine object.

	@return MainGameScene object.
  */
  ScenePtr CreateMainGameScene(Engine&) {
	return ScenePtr(new MainGameScene);
  }

} // namespace SunnySideUp