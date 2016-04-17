/** @file MainGameScene.cpp
*/
#include "LevelInfo.h"
#include "Scene.h"
#include "../Audio.h"
#include "../../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include <boost/math/constants/constants.hpp>
#include <array>
#include <tuple>

namespace SunnySideUp {

  using namespace Mai;

  enum DirectionKey {
	DIRECTIONKEY_UP,
	DIRECTIONKEY_LEFT,
	DIRECTIONKEY_DOWN,
	DIRECTIONKEY_RIGHT,
  };

  struct DebugData {
#ifndef NDEBUG
	DebugData()
	  : debug(false)
	  , dragging(false)
	  , mouseX(-1)
	  , mouseY(-1)
	  , camera(Position3F(0, 0, 0), Vector3F(0, 0, -1), Vector3F(0, 1, 0))
	{}

	void SetDebugObj(size_t i, const ObjectPtr& obj) {
	  if (i < debugObj.size()) {
		debugObj[i] = obj;
	  }
	}

	void PressMouseButton(int mx, int my) {
	  if (debug) {
		mouseX = mx;
		mouseY = my;
		dragging = true;
	  }
	}

	void ReleaseMouseButton() {
	  if (debug) {
		dragging = false;
	  }
	}

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
#ifdef SHOW_DEBUG_SENSOR_OBJECT
	ObjectPtr debugSensorObj;
#endif // SHOW_DEBUG_SENSOR_OBJECT
#else
	void SetDebugObj(size_t, const ObjectPtr&) {}
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
  static const int offsetTopObstructs = -200;

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
	for (int i = 1; i < count - 1; ++i) {
	  const Position3F p = start + distance * static_cast<float>(i) / static_cast<float>(count - 1);
	  const float theta = degreeToRadian(static_cast<float>(random() % 360));
	  const float r = static_cast<float>(random() % 190) + 10.0f;
	  const float tx = center.x + std::cos(theta) * r;
	  const float tz = center.z + std::sin(theta) * r;
	  v.emplace_back(p.x + tx, p.y, p.z + tz);
	  center = Position3F(tx, 0, tz) * (r - 200.0f) / r;
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
	const std::vector<Vector3F> controlPoints = CreateControlPoints(start, goal, (length + 999) / 1000 + 2, random);
	return CreateBSpline(controlPoints, (length + 99) / 100);
  }

  /** Control the main game play.

    Player start to fall down from the range of 4000m from 2000m.
	When player reaches 100m, it is success if the moving vector is facing
	within the range of target. Otherwise fail.
  */
  class MainGameScene : public Scene {
  public:
	MainGameScene()
	  : pPartitioner(new SpacePartitioner(region.min, region.max, unitRegionSize, maxObject))
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
	virtual bool Load(Engine& engine) {
	  directionKeyDownList.fill(false);
	  Renderer& renderer = engine.GetRenderer();

	  const int level = std::min(GetMaximumLevel(), engine.GetCommonData<CommonData>()->level);
	  const LevelInfo& levelInfo = GetLevelInfo(level);

	  const TimeOfScene tos = [levelInfo]() {
		if (levelInfo.hoursOfDay >= 7 && levelInfo.hoursOfDay < 16) {
		  return TimeOfScene_Noon;
		} else if (levelInfo.hoursOfDay >= 16 && levelInfo.hoursOfDay < 19) {
		  return TimeOfScene_Sunset;
		} else {
		  return TimeOfScene_Night;
		}
	  }();
      renderer.SetTimeOfScene(tos);

	  // The player character.
	  {
		auto obj = renderer.CreateObject("ChickenEgg", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		Object& o = *obj;
		o.SetAnimation(renderer.GetAnimation("Dive"));
		const Vector3F trans(5, static_cast<GLfloat>(levelInfo.startHeight), 4.5f);
		o.SetTranslation(trans);
		//o.SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(45), degreeToRadian<float>(0));
		//o.SetScale(Vector3F(5, 5, 5));
		Collision::RigidBodyPtr p(new Collision::SphereShape(trans.ToPosition3F(), 5.0f, 0.1f));
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
		const float distance = RandomFloat(50);
		const float tx = std::cos(theta) * distance;
		const float tz = std::sin(theta) * distance;
		o.SetTranslation(Vector3F(tx, 1, tz));
		pPartitioner->Insert(obj);
		objFlyingPan = obj;
	  }

	  {
		const std::vector<Position3F> modelRoute = CreateModelRoute(Position3F(5, static_cast<float>(levelInfo.startHeight), 4.5f), objFlyingPan->Position() + Vector3F(0, static_cast<float>(goalHeight), 0), random);
		const auto end = modelRoute.end() - 2;
		const float step = static_cast<float>(unitObstructsSize) * std::max(1.0f, (4.0f - static_cast<float>(levelInfo.difficulty) * 0.5f));
		const int density = std::min<int>(4, levelInfo.density);
		for (float height = static_cast<float>(levelInfo.startHeight + offsetTopObstructs); height > static_cast<float>(minObstructHeight); height -= step) {
		  auto itr = std::upper_bound(modelRoute.begin(), modelRoute.end(), height,
			[](float h, const Position3F& p) { return h > p.y; }
		  );
		  if (itr >= end) {
			continue;
		  }
		  const Position3F& e = *itr;
		  Vector3F axis(*(itr + 1) - e);
		  axis.y = 0;
		  if (axis.LengthSq() < 1.0f) {
			continue;
		  }
		  axis.Normalize();
		  axis *= RandomFloat(10) + 70.0f;
		  const Vector3F posList[] = {
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
			  o.SetRotation(rx, ry, rz);
			  Collision::RigidBodyPtr p(new Collision::BoxShape(trans.ToPosition3F(), o.RotTrans().rot, Vector3F(1, 1.5f, 1) * scale, scale * scale * scale * (5 * 6 * 5 / 3.0f)));
			  p->thrust = Vector3F(0, 9.8f, 0);
			  pPartitioner->Insert(obj, p);
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
			  Collision::RigidBodyPtr p(new Collision::BoxShape(trans.ToPosition3F(), o.RotTrans().rot, Vector3F(2.615f, 2.615f, 2.615f) * scale, scale * scale * scale * (5 * 6 * 5 / 3.0f)));
			  p->thrust = Vector3F(0, 9.8f, 0);
			  pPartitioner->Insert(obj, p);
			}
		  }
		}
	  }
	  for (float height = static_cast<float>(levelInfo.startHeight + offsetTopObstructs); height > static_cast<float>(minObstructHeight); height -= static_cast<float>(unitObstructsSize) * 5) {
		auto obj = renderer.CreateObject("block1", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		Object& o = *obj;
		const float h = RandomFloat(unitObstructsSize * 4) + static_cast<float>(unitObstructsSize) * 0.5f;
		const float theta = degreeToRadian<float>(RandomFloat(360));
		const float distance = RandomFloat(150);
		const Vector3F trans(std::cos(theta) * distance, height + h, std::sin(theta) * distance);
		o.SetTranslation(trans);
		const float scale = 5.0f;
		o.SetScale(Vector3F(scale, scale, scale));
		const float rx = degreeToRadian<float>(RandomFloat(90) - 45.0f);
		const float ry = degreeToRadian<float>(RandomFloat(360));
		o.SetRotation(rx, ry, 0);
		Collision::RigidBodyPtr p(new Collision::BoxShape(trans.ToPosition3F(), o.RotTrans().rot, Vector3F(5, 1, 4) * scale, 10 * 1 * 8));
		p->thrust = Vector3F(0, 9.8f, 0);
		pPartitioner->Insert(obj, p);
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
	  {
		auto obj = renderer.CreateObject("ground", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		Object& o = *obj;
		o.SetRotation(degreeToRadian(-90.0f), 0, 0);
		Collision::RigidBodyPtr p(new Collision::PlaneShape(Position3F(0, 0, 0), Vector3F(0, 1, 0), 1000 * 1000 * 1000));
		p->thrust = Vector3F(0, 9.8f, 0);
		pPartitioner->Insert(obj, p);
	  }

	  // Add cloud.
	  {
		for (int i = 0; i < 5; ++i) {
		  const int cloudCount = ((i * i / 2 + 1) * levelInfo.cloudage) / levelInfo.maxCloudage;
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
			auto obj = renderer.CreateObject(idList[index], Material(Color4B(255, 255, 255, 64), 0, 0), "cloud");
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
	  return true;
	}

	virtual bool Unload(Engine& engine) {
	  pPartitioner->Clear();
	  status = STATUSCODE_STOPPED;
	  return true;
	}

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
        const float tz = e.Tilt.Z - 0.0f;
        if (std::abs(tz) < 0.1f) {
          playerMovement.x = 0.0f;
        } else if (tz < 0.0f) {
          playerMovement.x = -(tz * 2.0f + 0.1f);
        } else {
          playerMovement.x = -(tz * 2.0f - 0.1f);
        }
		const float ty = e.Tilt.Y - 0.1f;
        if (std::abs(ty) < 0.1f) {
          playerMovement.z = 0.0f;
        } else if (ty < 0.0f) {
          playerMovement.z = e.Tilt.Y * 2.0f + 0.1f;
        } else {
          playerMovement.z = e.Tilt.Y * 2.0f - 0.1f;
        }
        break;
      }
	  default:
		break;
	  }
	}

	virtual int Update(Engine& engine, float deltaTime) {
	  return (this->*updateFunc)(engine, deltaTime);
	}

	/** update game play.

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
		playerMovement.z = 0.0f;
		if (directionKeyDownList[DIRECTIONKEY_UP]) {
		  playerMovement.z += 1.0f;
		}
		if (directionKeyDownList[DIRECTIONKEY_DOWN]) {
		  playerMovement.z -= 1.0f;
		}
		playerMovement.x = 0.0f;
		if (directionKeyDownList[DIRECTIONKEY_LEFT]) {
		  playerMovement.x += 1.0f;
		}
		if (directionKeyDownList[DIRECTIONKEY_RIGHT]) {
		  playerMovement.x -= 1.0f;
		}
		rigidCamera->accel += playerMovement;
		playerRotation.x = std::min(0.5f, std::max(-0.5f, playerRotation.x - playerMovement.z * 0.05f));
		playerRotation.z = std::min(0.5f, std::max(-0.5f, playerRotation.z + playerMovement.x * 0.05f));
		objPlayer->SetRotation(playerRotation.x, playerRotation.y, playerRotation.z);
#else
		rigidCamera->accel += playerMovement;
		playerRotation.x = std::min(0.5f, std::max(-0.5f, playerMovement.z * -0.5f));
		playerRotation.z = std::min(0.5f, std::max(-0.5f, playerMovement.x * 0.5f));
		objPlayer->SetRotation(playerRotation.x, playerRotation.y, playerRotation.z);
#endif // __ANDROID__
	  }
	  pPartitioner->Update(deltaTime);
	  if (rigidCamera && rigidCamera->hasLatestCollision) {
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
		renderer.Update(deltaTime, rigidCamera->Position() + Vector3F(0, 20, 2), Vector3F(0, -1, 0), Vector3F(0, 0, 1));
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

	Transition to the scene of the success/failure event when the fadeout finished.
	This is the update function called from Update().
	*/
	int DoFadeOut(Engine& engine, float deltaTime) {
	  Renderer& renderer = engine.GetRenderer();
	  if (!debugData.Updata(renderer, deltaTime)) {
		renderer.Update(deltaTime, rigidCamera->Position() + Vector3F(0, 20, 2), Vector3F(0, -1, 0), Vector3F(0, 0, 1));
	  }
	  if (renderer.GetCurrentFilterMode() == Renderer::FILTERMODE_NONE) {
		engine.GetCommonData<CommonData>()->currentTime = static_cast<int64_t>(stopWatch * 1000.0f);
		renderer.FadeIn(1.0f);
		const float scale = objFlyingPan->Scale().x;
		const Vector3F v = objPlayer->Position() - objFlyingPan->Position();
		const float distance = std::sqrt(v.x * v.x + v.z + v.z);
		if (distance <= 1.25f * scale) {
		  engine.GetAudio().PlaySE("success", 1.0f);
		  return SCENEID_SUCCESS;
		}
		engine.GetAudio().PlaySE("failure", 1.0f);
		return SCENEID_FAILURE;
	  }
	  return SCENEID_CONTINUE;
	}

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
		char buf[32];
		sprintf(buf, "%03.0fKm/h", rigidCamera->accel.Length() * 3.6f);
		const float width0 = renderer.GetStringWidth(buf) * 0.8f;
		renderer.AddString(0.95f - width0, 0.05f, 0.8f, Color4B(255, 255, 255, 255), buf);

		float y = objPlayer->Position().y;
		if (auto radius = rigidCamera->GetRadius()) {
		  y -= *radius;
		}
		sprintf(buf, "%04.0fm", y);
		const float width1 = renderer.GetStringWidth(buf) * 0.8f;
		renderer.AddString(0.95f - width1, 0.1f, 0.8f, Color4B(255, 255, 255, 255), buf);

		sprintf(buf, "%03.3fs", stopWatch);
		const float width2 = renderer.GetStringWidth(buf) * 0.8f;
		renderer.AddString(0.95f - width2, 0.15f, 0.8f, Color4B(255, 255, 255, 255), buf);

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

  ScenePtr CreateMainGameScene(Engine&) {
	return ScenePtr(new MainGameScene);
  }

} // namespace SunnySideUp