/** @file MainGameScene.cpp
*/
#include "Scene.h"
#include "../../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include <boost/math/constants/constants.hpp>
#include <array>

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
	  , camera(Position3F(0, 0, 0), Vector3F(0, 0, -1), Vector3F(0, 1, 0))
	  , mouseX(-1)
	  , mouseY(-1)
	{}

	void SetDebugObj(size_t i, const ObjectPtr& obj) {
	  if (i >= 0 && i < debugObj.size()) {
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

  const struct {
	int min;
	int max;
  } startingHeightRange = { 2000, 4000 };
  static const int goalHeight = 100;

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
#if 1
	  // ランダムにオブジェクトを発生させてみる.
	  const int regionCount = static_cast<int>(std::ceil((region.max.y - region.min.y) / unitRegionSize));
	  for (int i = 0; i < regionCount; ++i) {
		int objectCount = random() % 2 + 1;
		while (--objectCount >= 0) {
		  const float theta = degreeToRadian<float>(RandomFloat(360));
		  const float distance = RandomFloat(200);
		  const float tx = std::cos(theta) * distance;
		  const float tz = std::sin(theta) * distance;
		  const float ty = RandomFloat(100);
		  if (boost::random::uniform_int_distribution<>(0, 99)(random) < 70) {
			auto obj = renderer.CreateObject("FlyingRock", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
			Object& o = *obj;
			const Vector3F trans(tx, ty + i * unitRegionSize, tz);
			o.SetTranslation(trans);
			const float scale = RandomFloat(40) / 10.0f + 1.0f;
			o.SetScale(Vector3F(scale, scale, scale));
			const float rx = degreeToRadian<float>(RandomFloat(30));
			const float ry = degreeToRadian<float>(RandomFloat(360));
			const float rz = degreeToRadian<float>(RandomFloat(30));
			o.SetRotation(rx, ry, rz);
			Collision::RigidBodyPtr p(new Collision::BoxShape(trans.ToPosition3F(), o.RotTrans().rot, Vector3F(2.5, 3, 2.5) * scale, scale * scale * scale * (5 * 6 * 5 / 3.0f)));
			p->thrust = Vector3F(0, 9.8f, 0);
			pPartitioner->Insert(obj, p, Vector3F(-1.47f, 0.25f, 2.512f) * scale);
		  } else {
			auto obj = renderer.CreateObject("block1", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
			Object& o = *obj;
			const Vector3F trans(tx, ty + i * unitRegionSize, tz);
			o.SetTranslation(trans);
			//		  o.SetScale(Vector3F(2, 2, 2));
			const float rx = degreeToRadian<float>(RandomFloat(360));
			const float ry = degreeToRadian<float>(RandomFloat(360));
			const float rz = degreeToRadian<float>(RandomFloat(360));
			o.SetRotation(rx, ry, rz);
			Collision::RigidBodyPtr p(new Collision::BoxShape(trans.ToPosition3F(), o.RotTrans().rot, Vector3F(5, 1, 4) * 2, 10 * 1 * 8));
			p->thrust = Vector3F(0, 9.8f, 0);
			pPartitioner->Insert(obj, p, Vector3F(0, 0, -1) * 2);
		  }
		}
	  }
#endif
	  // The player character.
	  {
		auto obj = renderer.CreateObject("ChickenEgg", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		Object& o = *obj;
		o.SetAnimation(renderer.GetAnimation("Dive"));
		const Vector3F trans(5, startingHeightRange.min, 4.5f);
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
		auto obj = renderer.CreateObject("block1", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		debugData.SetDebugObj(0, obj);
		Object& o = *obj;
		//	  o.SetScale(Vector3F(2, 2, 2));
		o.SetTranslation(Vector3F(-15, 100, 0));
		o.SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(0), degreeToRadian<float>(0));
		pPartitioner->Insert(obj);
	  }
#if 0
	  {
		auto obj = renderer.CreateObject("SunnySideUp", Material(Color4B(255, 255, 255, 255), 0.1f, 0), "default");
		debugData.SetDebugObj(1, obj);
		Object& o = *obj;
		o.SetScale(Vector3F(3, 3, 3));
		o.SetTranslation(Vector3F(15, 11, 0));
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

	  if (0) {
		auto obj0 = renderer.CreateObject("BrokenEgg", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		Object& o0 = *obj0;
		o0.SetScale(Vector3F(3, 3, 3));
		o0.SetTranslation(Vector3F(-12, 0.75f, 0));
		pPartitioner->Insert(obj0);
		auto obj2 = renderer.CreateObject("EggYolk", Material(Color4B(255, 255, 255, 255), -0.05f, 0), "default");
		Object& o2 = *obj2;
		o2.SetScale(Vector3F(3, 3, 3));
		o2.SetTranslation(Vector3F(-15.0f, 0.7f, 0.0));
		pPartitioner->Insert(obj2);
		auto obj1 = renderer.CreateObject("EggWhite", Material(Color4B(255, 255, 255, 64), -0.5f, 0), "defaultWithAlpha");
		Object& o1 = *obj1;
		o1.SetScale(Vector3F(3, 3, 3));
		o1.SetTranslation(Vector3F(-15, 1, 0));
		pPartitioner->Insert(obj1);
	  }

#if 1
	  for (int i = 0; i < 5; ++i) {
		const int cloudCount = i * i / 2 + 1;
		const int heightMax = 2600 - i * 400;
		const int heightMin = heightMax - 400;
		const int radiusMax = (i * i + 4) * 50;
		const int radiusMax2 = radiusMax * radiusMax;
		for (int j = 0; j < cloudCount; ++j) {
		  static const char* const idList[] = { "cloud0", "cloud1", "cloud2", "cloud3" };
		  const int index = boost::random::uniform_int_distribution<>(0, 3)(random);
		  const int y = boost::random::uniform_int_distribution<>(heightMin, heightMax)(random);
		  int r = boost::random::uniform_int_distribution<>(10, radiusMax)(random);
		  //r = radiusMax - r * r / radiusMax2;
		  const int a0 = boost::random::uniform_int_distribution<>(0, 359)(random);
		  const int a1 = boost::random::uniform_int_distribution<>(30, 60)(random);
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
#endif
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
		}
		break;
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
		{
		  playerRotation.x = std::min(0.5f, std::max(-0.5f, playerRotation.x - playerMovement.z * 0.05f));
		  playerRotation.z = std::min(0.5f, std::max(-0.5f, playerRotation.z + playerMovement.x * 0.05f));
		  objPlayer->SetRotation(playerRotation.x, playerRotation.y, playerRotation.z);
		}
	  }
	  pPartitioner->Update(deltaTime);
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
		renderer.FadeIn(1.0f);
		const float scale = objFlyingPan->Scale().x;
		const Vector3F v = objPlayer->Position() - objFlyingPan->Position();
		const float distance = std::sqrt(v.x * v.x + v.z + v.z);
		if (distance <= 1.0f * scale) {
		  return SCENEID_SUCCESS;
		}
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