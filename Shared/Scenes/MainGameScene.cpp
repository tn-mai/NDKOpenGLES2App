#include "Scene.h"
#include "../../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include <boost/math/constants/constants.hpp>
#include <array>

namespace SunnySideUp {

  using namespace Mai;

  static const Region region = { Position3F(-100, 200, -100), Position3F(100, 3800, 100) };
  static const float unitRegionSize = 100.0f;
  static const size_t maxObject = 1000;

  class MainGameScene : public Scene {
  public:
	MainGameScene()
	  : pPartitioner(new SpacePartitioner(region.min, region.max, unitRegionSize, maxObject))
	  , random(static_cast<uint32_t>(time(nullptr)))
#ifndef NDEBUG
	  , debug(true)
	  , dragging(false)
	  , camera(Position3F(0, 0, 0), Vector3F(0, 0, -1), Vector3F(0, 1, 0))
	  , mouseX(-1)
	  , mouseY(-1)
#endif // NDEBUG
	{
	  directionKeyDownList.fill(false);
	}
	virtual ~MainGameScene() {}
	virtual bool Load(Engine& engine) {		
	  directionKeyDownList.fill(false);
	  Renderer& renderer = engine.GetRenderer();
#if 0
	  // ランダムにオブジェクトを発生させてみる.
	  const int regionCount = std::ceil((region.max.y - region.min.y) / unitRegionSize);
	  for (int i = 0; i < regionCount; ++i) {
		int objectCount = random() % 2 + 1;
		while (--objectCount >= 0) {
		  const float theta = degreeToRadian<float>(random() % 360);
		  const float distance = random() % 200;
		  const float tx = std::cos(theta) * distance;
		  const float tz = std::sin(theta) * distance;
		  const float ty = random() % 100;
		  if (boost::random::uniform_int_distribution<>(0, 99)(random) < 70) {
			auto obj = renderer.CreateObject("FlyingRock", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
			Object& o = *obj;
			const Vector3F trans(tx, ty + i * unitRegionSize, tz);
			o.SetTranslation(trans);
			const float scale = static_cast<float>(random() % 40) / 10.0f + 1.0f;
			o.SetScale(Vector3F(scale, scale, scale));
			const float rx = degreeToRadian<float>(random() % 30);
			const float ry = degreeToRadian<float>(random() % 360);
			const float rz = degreeToRadian<float>(random() % 30);
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
			const float rx = degreeToRadian<float>(random() % 360);
			const float ry = degreeToRadian<float>(random() % 360);
			const float rz = degreeToRadian<float>(random() % 360);
			o.SetRotation(rx, ry, rz);
			Collision::RigidBodyPtr p(new Collision::BoxShape(trans.ToPosition3F(), o.RotTrans().rot, Vector3F(5, 1, 4) * 2, 10 * 1 * 8));
			p->thrust = Vector3F(0, 9.8f, 0);
			pPartitioner->Insert(obj, p, Vector3F(0, 0, -1) * 2);
		  }
		}
	  }
#endif
	  {
		auto obj = renderer.CreateObject("ChickenEgg", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		Object& o = *obj;
		o.SetAnimation(renderer.GetAnimation("Walk"));
		const Vector3F trans(5, 10, 4.5f);
		o.SetTranslation(trans);
		//o.SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(45), degreeToRadian<float>(0));
		o.SetScale(Vector3F(5, 5, 5));
		Collision::RigidBodyPtr p(new Collision::SphereShape(trans.ToPosition3F(), 5.0f, 0.1f));
		p->thrust = Vector3F(0, 9.8f, 0);
		pPartitioner->Insert(obj, p, Vector3F(0, 0, 0));
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
		debugObj[0] = renderer.CreateObject("block1", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		Object& o = *debugObj[0];
		//	  o.SetScale(Vector3F(2, 2, 2));
		o.SetTranslation(Vector3F(-15, 100, 0));
		o.SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(0), degreeToRadian<float>(0));
		pPartitioner->Insert(debugObj[0]);
	  }
#if 0
	  {
		auto obj = renderer.CreateObject("FlyingPan", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		Object& o = *obj;
		o.SetScale(Vector3F(5, 5, 5));
		o.SetTranslation(Vector3F(15, 10, 0));
		pPartitioner->Insert(obj);
	  }
	  {
		debugObj[1] = renderer.CreateObject("SunnySideUp", Material(Color4B(255, 255, 255, 255), 0.1f, 0), "default");
		Object& o = *debugObj[1];
		o.SetScale(Vector3F(3, 3, 3));
		o.SetTranslation(Vector3F(15, 11, 0));
		pPartitioner->Insert(debugObj[1]);
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

#if 0
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
		  mouseX = e.MouseButton.X;
		  mouseY = e.MouseButton.Y;
		  dragging = true;
		}
		break;
	  case Event::EVENT_MOUSE_BUTTON_RELEASED:
		if (e.MouseButton.Button == MOUSEBUTTON_LEFT) {
		  dragging = false;
		}
		break;
	  case Event::EVENT_MOUSE_ENTERED:
		break;
	  case Event::EVENT_MOUSE_LEFT:
		dragging = false;
		break;
	  case Event::EVENT_MOUSE_MOVED: {
		if (dragging) {
		  const float x = static_cast<float>(mouseX - e.MouseMove.X) * 0.005f;
		  const float y = static_cast<float>(mouseY - e.MouseMove.Y) * 0.005f;
		  const Vector3F leftVector = camera.eyeVector.Cross(camera.upVector).Normalize();
		  camera.eyeVector = (Quaternion(camera.upVector, x) * Quaternion(leftVector, y)).Apply(camera.eyeVector).Normalize();
		  camera.upVector = Quaternion(leftVector, y).Apply(camera.upVector).Normalize();
		}
		mouseX = e.MouseMove.X;
		mouseY = e.MouseMove.Y;
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
#if 1
#ifndef NDEBUG
	  if (directionKeyDownList[DIRECTIONKEY_UP]) {
		camera.position += camera.eyeVector * 0.25f;
	  };
	  if (directionKeyDownList[DIRECTIONKEY_DOWN]) {
		camera.position -= camera.eyeVector * 0.25f;
	  }
	  if (directionKeyDownList[DIRECTIONKEY_LEFT]) {
		camera.position -= camera.eyeVector.Cross(camera.upVector) * 0.25f;
	  }
	  if (directionKeyDownList[DIRECTIONKEY_RIGHT]) {
		camera.position += camera.eyeVector.Cross(camera.upVector) * 0.25f;
	  }
	  //debugCamera.Update(deltaTime, fusedOrientation);
#endif // NDEBUG
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
	  engine.GetRenderer().Update(deltaTime, camera.position, camera.eyeVector, camera.upVector);
	  //			for (auto&& e : engine.obj) {
	  //				e.Update(engine.deltaTime);
	  //				while (e.GetCurrentTime() >= 1.0f) {
	  //					e.SetCurrentTime(e.GetCurrentTime() - 1.0f);
	  //				}
	  //			}

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
		sprintf(buf, "%03.1fKm/h", rigidCamera->accel.Length() * 3.6f);
		renderer.AddDebugString(8, 40, buf);
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

  private:
	std::unique_ptr<SpacePartitioner> pPartitioner;
	boost::random::mt19937 random;

#ifdef SHOW_DEBUG_SENSOR_OBJECT
	ObjectPtr debugSensorObj;
#endif // SHOW_DEBUG_SENSOR_OBJECT
	ObjectPtr debugObj[3];
	Collision::RigidBodyPtr rigidCamera;

	enum DirectionKey {
	  DIRECTIONKEY_UP,
	  DIRECTIONKEY_LEFT,
	  DIRECTIONKEY_DOWN,
	  DIRECTIONKEY_RIGHT,
	};
	std::array<bool, 4> directionKeyDownList;

#ifndef NDEBUG
	bool debug;
	bool dragging;
	Camera camera;
	int mouseX;
	int mouseY;
#endif // NDEBUG
  };

  ScenePtr CreateMainGameScene(Engine&) {
	return ScenePtr(new MainGameScene);
  }

} // namespace SunnySideUp