#include "Engine.h"
#include <boost/math/constants/constants.hpp>
#include <time.h>

#if 0
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/sensor.h>
#include <android/window.h>

#include <typeinfo>
#include <boost/random.hpp>
#include <vector>

#include <fcntl.h>
#include <linux/ioctl.h>
#include <linux/android_alarm.h>
#endif

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))
#else
#define LOGI(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#define LOGE(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#endif // __ANDROID__

namespace Mai {

  static const Region region = { Position3F(-100, 200, -100), Position3F(100, 3800, 100) };
  static const float unitRegionSize = 100.0f;
  static const size_t maxObject = 1000;

  Engine::Engine(Window* p)
	: initialized(false)
	, pWindow(p)
	, renderer()
	, pScene(new Scene(region.min, region.max, unitRegionSize, maxObject))
	, random(time(nullptr))

	, avgFps(30.0f)
	, deltaTime(1.0f / avgFps)
	, frames(0)
	, latestFps(30)
	, startTime(0)

#ifndef NDEBUG
	, debug(true)
	, dragging(false)
	, camera(Position3F(0, 0, 0), Vector3F(0, 0, -1), Vector3F(0, 1, 0))
	, mouseX(-1)
	, mouseY(-1)
#endif // NDEBUG
  {
  }

  template<typename T>
  T degreeToRadian(T d) {
	return d * boost::math::constants::pi<T>() / static_cast<T>(180);
  }

  /**
  * 現在の表示の EGL コンテキストを初期化します。
  */
  void Engine::InitDisplay() {
	// GL の状態を初期化します。
	renderer.Initialize(*pWindow);
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
		  pScene->Insert(obj, p, Vector3F(-1.47f, 0.25f, 2.512f) * scale);
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
		  pScene->Insert(obj, p, Vector3F(0, 0, -1) * 2);
		}
	  }
	}
#endif
	{
	  auto obj = renderer.CreateObject("ChickenEgg", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  Object& o = *obj;
	  o.SetAnimation(renderer.GetAnimation("Walk"));
	  const Vector3F trans(5, 100, 4.5f);
	  o.SetTranslation(trans);
	  //o.SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(45), degreeToRadian<float>(0));
	  o.SetScale(Vector3F(5, 5, 5));
	  Collision::RigidBodyPtr p(new Collision::SphereShape(trans.ToPosition3F(), 5.0f, 0.1f));
	  p->thrust = Vector3F(0, 9.8f, 0);
	  pScene->Insert(obj, p, Vector3F(0, 0, 0));
	}
#if 0
	{
	  auto obj = renderer.CreateObject("Accelerator", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  Object& o = *obj;
	  o.SetScale(Vector3F(5, 5, 5));
	  o.SetTranslation(Vector3F(-15, 120, 0));
	  o.SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(0), degreeToRadian<float>(0));
	  pScene->Insert(obj);
	}
#endif
	{
	  debugObj[0] = renderer.CreateObject("block1", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  Object& o = *debugObj[0];
	  //	  o.SetScale(Vector3F(2, 2, 2));
	  o.SetTranslation(Vector3F(-15, 100, 0));
	  o.SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(0), degreeToRadian<float>(0));
	  pScene->Insert(debugObj[0]);
	}
#if 0
	{
	  auto obj = renderer.CreateObject("FlyingPan", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  Object& o = *obj;
	  o.SetScale(Vector3F(5, 5, 5));
	  o.SetTranslation(Vector3F(15, 10, 0));
	  pScene->Insert(obj);
	}
	{
	  debugObj[1] = renderer.CreateObject("SunnySideUp", Material(Color4B(255, 255, 255, 255), 0.1f, 0), "default");
	  Object& o = *debugObj[1];
	  o.SetScale(Vector3F(3, 3, 3));
	  o.SetTranslation(Vector3F(15, 11, 0));
	  pScene->Insert(debugObj[1]);
	}
#endif
	{
	  auto obj = renderer.CreateObject("ground", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  Object& o = *obj;
	  o.SetRotation(degreeToRadian(-90.0f), 0, 0);
	  Collision::RigidBodyPtr p(new Collision::PlaneShape(Position3F(0, 0, 0), Vector3F(0, 1, 0), 1000 * 1000 * 1000));
	  p->thrust = Vector3F(0, 9.8f, 0);
	  pScene->Insert(obj, p);
	}

	if (0) {
	  auto obj0 = renderer.CreateObject("BrokenEgg", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  Object& o0 = *obj0;
	  o0.SetScale(Vector3F(3, 3, 3));
	  o0.SetTranslation(Vector3F(-12, 0.75f, 0));
	  pScene->Insert(obj0);
	  auto obj2 = renderer.CreateObject("EggYolk", Material(Color4B(255, 255, 255, 255), -0.05f, 0), "default");
	  Object& o2 = *obj2;
	  o2.SetScale(Vector3F(3, 3, 3));
	  o2.SetTranslation(Vector3F(-15.0f, 0.7f, 0.0));
	  pScene->Insert(obj2);
	  auto obj1 = renderer.CreateObject("EggWhite", Material(Color4B(255, 255, 255, 64), -0.5f, 0), "defaultWithAlpha");
	  Object& o1 = *obj1;
	  o1.SetScale(Vector3F(3, 3, 3));
	  o1.SetTranslation(Vector3F(-15, 1, 0));
	  pScene->Insert(obj1);
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
		pScene->Insert(obj);
	  }
	}
#endif

#ifdef SHOW_DEBUG_SENSOR_OBJECT
	debugSensorObj = renderer.CreateObject("octahedron", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
#endif // SHOW_DEBUG_SENSOR_OBJECT

	LOGI("engine_init_display");
	initialized = true;
  }

  int_fast64_t get_time() {
#ifdef __ANDROID__
#if 1
	timespec tmp;
	clock_gettime(CLOCK_MONOTONIC, &tmp);
	return tmp.tv_sec * (1000UL * 1000UL * 1000UL) + tmp.tv_nsec;
#else
	static int sfd = -1;
	if (sfd == -1) {
	  sfd = open("/dev/alarm", O_RDONLY);
	}
	timespec ts;
	const int result = ioctl(sfd, ANDROID_ALARM_GET_TIME(ANDROID_ALARM_ELAPSED_REALTIME), &ts);
	if (result != 0) {
	  const int err = clock_gettime(CLOCK_MONOTONIC, &ts);
	  if (err < 0) {
		LOGI("ERROR(%d) from; clock_gettime(CLOCK_MONOTONIC)", err);
	  }
	}
	return ts.tv_sec * (1000 * 1000 * 1000) + ts.tv_nsec;
#endif
#else
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return ft.dwHighDateTime * 10000000LL + ft.dwLowDateTime * 100LL;
#endif // __ANDROID__
  }

  Engine::State Engine::Update(Window* pWindow, float tick) {
	while (auto e = pWindow->PopEvent()) {
	  switch (e->Type) {
	  case Event::EVENT_CLOSED:
		TermDisplay();
		return STATE_TERMINATE;
	  case Event::EVENT_INIT_WINDOW:
		InitDisplay();
		break;
	  case Event::EVENT_TERM_WINDOW:
		TermDisplay();
		break;
	  case Event::EVENT_MOUSE_BUTTON_PRESSED:
		if (e->MouseButton.Button == MOUSEBUTTON_LEFT) {
		  mouseX = e->MouseButton.X;
		  mouseY = e->MouseButton.Y;
		  dragging = true;
		}
		break;
	  case Event::EVENT_MOUSE_BUTTON_RELEASED:
		if (e->MouseButton.Button == MOUSEBUTTON_LEFT) {
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
		  const float x = static_cast<float>(mouseX - e->MouseMove.X) * 0.005f;
		  const float y = static_cast<float>(mouseY - e->MouseMove.Y) * 0.005f;
		  const Mai::Vector3F leftVector = camera.eyeVector.Cross(camera.upVector).Normalize();
		  camera.eyeVector = (Mai::Quaternion(camera.upVector, x) * Mai::Quaternion(leftVector, y)).Apply(camera.eyeVector).Normalize();
		  camera.upVector = Mai::Quaternion(leftVector, y).Apply(camera.upVector).Normalize();
		}
		mouseX = e->MouseMove.X;
		mouseY = e->MouseMove.Y;
		break;
	  }
	  case Event::EVENT_KEY_PRESSED:
		switch (e->Key.Code) {
		case KEY_W:
		  camera.position += camera.eyeVector;
		  break;
		case KEY_S:
		  camera.position -= camera.eyeVector;
		  break;
		}
		break;
	  default:
		break;
	  }
	}
	return STATE_CONTINUE;
  }

  void Engine::DrawFrame() {
	if (initialized) {
	  // イベントが完了したら次のアニメーション フレームを描画します。
#if 1
	  ++frames;
	  const int64_t curTime = get_time();
	  if (curTime < startTime) {
		startTime = curTime;
		frames = 0;
	  } else if (curTime - startTime >= (1000UL * 1000UL * 1000UL)) {
		latestFps = std::min(frames, 60);
		startTime = curTime;
		frames = 0;
		if (latestFps > avgFps * 0.3f) { // 低すぎるFPSはノイズとみなす.
		  avgFps = (avgFps + latestFps) * 0.5f;
		  deltaTime = (deltaTime + 1.0f / latestFps) * 0.5f;
		}
	  }
#endif
#if 1
	  //debugCamera.Update(deltaTime, fusedOrientation);
	  pScene->Update(deltaTime);
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
	  pScene->Update(engine.deltaTime);
	  if (rigidCamera) {
		Position3F pos = rigidCamera->Position();
		pos.y += 10.0f;
		debugCamera.Position(pos);
	  }
#endif
	  renderer.Update(deltaTime, camera.position, camera.eyeVector, camera.upVector);
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
	  Draw();
	}
  }
  
  /**
  * 現在ディスプレイに関連付けられている EGL コンテキストを削除します。
  */
  void Engine::TermDisplay() {
	initialized = false;
	renderer.Unload();
	pScene->Clear();
	LOGI("engine_term_display");
  }

  /**
  * ディスプレイ内の現在のフレームのみ。
  */
  void Engine::Draw() {
	if (!renderer.HasDisplay() || !initialized) {
	  // ディスプレイがありません。
	  return;
	}

	std::vector<ObjectPtr> objList;
	objList.reserve(10 * 10);
#if 0
	float posY = debugCamera.Position().y;
	if (debugCamera.EyeVector().y >= 0.0f) {
	  posY = std::max(region.min.y, posY - unitRegionSize * 2);
	  for (int i = 0; i < 6; ++i) {
		const Cell& cell = pScene->GetCell(posY);
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
		const Cell& cell = pScene->GetCell(posY);
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
	for (auto itr = pScene->Begin(); itr != pScene->End(); ++itr) {
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

	renderer.ClearDebugString();
	char buf[32];
#if 0
	sprintf(buf, "OX:%1.3f", fusedOrientation.x);
	renderer.AddDebugString(8, 800 - 16 * 9, buf);
	sprintf(buf, "OY:%1.3f", fusedOrientation.y);
	renderer.AddDebugString(8, 800 - 16 * 8, buf);
	sprintf(buf, "OZ:%1.3f", fusedOrientation.z);
	renderer.AddDebugString(8, 800 - 16 * 7, buf);

	sprintf(buf, "MX:%1.3f", magnet.x);
	renderer.AddDebugString(8, 800 - 16 * 6, buf);
	sprintf(buf, "MY:%1.3f", magnet.y);
	renderer.AddDebugString(8, 800 - 16 * 5, buf);
	sprintf(buf, "MZ:%1.3f", magnet.z);
	renderer.AddDebugString(8, 800 - 16 * 4, buf);

	sprintf(buf, "AX:%1.3f", accel.x);
	renderer.AddDebugString(8, 800 - 16 * 3, buf);
	sprintf(buf, "AY:%1.3f", accel.y);
	renderer.AddDebugString(8, 800 - 16 * 2, buf);
	sprintf(buf, "AZ:%1.3f", accel.z);
	renderer.AddDebugString(8, 800 - 16 * 1, buf);
#endif
	sprintf(buf, "FPS:%02d", latestFps);
	renderer.AddDebugString(8, 8, buf);
	sprintf(buf, "AVG:%02.1f", avgFps);
	renderer.AddDebugString(8, 24, buf);
	if (rigidCamera) {
	  sprintf(buf, "%03.1fKm/h", rigidCamera->accel.Length() * 3.6f);
	  renderer.AddDebugString(8, 40, buf);
	}

	renderer.Render(&objList[0], &objList[0] + objList.size());
	renderer.Swap();
  }

} // namespace Mai