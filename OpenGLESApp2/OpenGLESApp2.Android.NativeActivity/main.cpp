/*
* Copyright (C) 2010 The Android Open Source Project
*
* Apache License Version 2.0 (「本ライセンス」) に基づいてライセンスされます。;
* 本ライセンスに準拠しない場合はこのファイルを使用できません。
* 本ライセンスのコピーは、以下の場所から入手できます。
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* 適用される法令または書面での同意によって命じられない限り、本ライセンスに基づいて頒布されるソフトウェアは、
* 明示黙示を問わず、いかなる保証も条件もなしに現状のまま
* 頒布されます。
* 本ライセンスでの権利と制限を規定した文言については、
* 本ライセンスを参照してください。
*
*/

#include "AndroidWindow.h"
#include "Scene.h"
#include "Renderer.h"
#include "android_native_app_glue.h"
#include "TouchSwipeCamera.h"
#include "../../Common/File.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/sensor.h>
#include <android/log.h>
#include <android/window.h>

#include <typeinfo>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/random.hpp>
#include <vector>
#include <streambuf>

#include <time.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <linux/android_alarm.h>

namespace BPT = boost::property_tree;

namespace Mai {

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))

  //#define SHOW_DEBUG_SENSOR_OBJECT

  static const Region region = { Position3F(-100, 200, -100), Position3F(100, 3800, 100) };
  static const float unitRegionSize = 100.0f;
  static const size_t maxObject = 1000;

  /**
  * 保存状態のデータです。
  */
  struct saved_state {
	float angle;
	int32_t x;
	int32_t y;
	saved_state() : angle(0), x(0), y(0) {}
  };

  /**
  * アプリの保存状態です。
  *
  * A sensor managiment code was referring to http://plaw.info/2012/03/android-sensor-fusion-tutorial/
  */
  struct Engine {
	Engine(android_app* app)
	  : initialized(false)
	  , window(app)
	  , renderer()
	  , pScene(new Scene(region.min, region.max, unitRegionSize, maxObject))
	  , random(time(nullptr))
	  , avgFps(30.0f)
	  , deltaTime(1.0f / avgFps)
	  , frames(0)
	  , latestFps(30)
	  , startTime(0)
	{
	}

	bool initialized;

	AndroidWindow window;
	Renderer renderer;
	std::unique_ptr<Scene> pScene;
	boost::random::mt19937 random;
#ifdef SHOW_DEBUG_SENSOR_OBJECT
	ObjectPtr debugSensorObj;
#endif // SHOW_DEBUG_SENSOR_OBJECT
	ObjectPtr debugObj[3];
	Collision::RigidBodyPtr rigidCamera;

	float avgFps;
	float deltaTime;
	int frames;
	int latestFps;
	int64_t startTime;
  };

  template<typename T>
  T degreeToRadian(T d) {
	return d * boost::math::constants::pi<T>() / static_cast<T>(180);
  }

  /**
  * 現在の表示の EGL コンテキストを初期化します。
  */
  static int engine_init_display(Engine* engine) {
	// OpenGL ES と EGL の初期化
#if 0
	/*4
	* 目的の構成の属性をここで指定します。
	* 以下で、オンスクリーン ウィンドウと
	* 互換性のある、各色最低 8 ビットのコンポーネントの EGLConfig を選択します
	*/
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};
	EGLint w, h, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, 0, 0);

	/* ここで、アプリケーションは目的の構成を選択します。このサンプルでは、
	* 抽出条件と一致する最初の EGLConfig を
	* 選択する単純な選択プロセスがあります */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID は、ANativeWindow_setBuffersGeometry() に
	* よって受け取られることが保証されている EGLConfig の属性です。
	* EGLConfig を選択したらすぐに、ANativeWindow バッファーを一致させるために
	* EGL_NATIVE_VISUAL_ID を使用して安全に再構成できます。*/
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
	context = eglCreateContext(display, config, NULL, NULL);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
	  LOGW("Unable to eglMakeCurrent");
	  return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w;
	engine->height = h;
	engine->state.angle = 0;
#endif
	// GL の状態を初期化します。
	engine->renderer.Initialize(engine->window);
#if 0
	// ランダムにオブジェクトを発生させてみる.
	const int regionCount = std::ceil((region.max.y - region.min.y) / unitRegionSize);
	for (int i = 0; i < regionCount; ++i) {
	  int objectCount = engine->random() % 2 + 1;
	  while (--objectCount >= 0) {
		const float theta = degreeToRadian<float>(engine->random() % 360);
		const float distance = engine->random() % 200;
		const float tx = std::cos(theta) * distance;
		const float tz = std::sin(theta) * distance;
		const float ty = engine->random() % 100;
		if (boost::random::uniform_int_distribution<>(0, 99)(engine->random) < 70) {
		  auto obj = engine->renderer.CreateObject("FlyingRock", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		  Object& o = *obj;
		  const Vector3F trans(tx, ty + i * unitRegionSize, tz);
		  o.SetTranslation(trans);
		  const float scale = static_cast<float>(engine->random() % 40) / 10.0f + 1.0f;
		  o.SetScale(Vector3F(scale, scale, scale));
		  const float rx = degreeToRadian<float>(engine->random() % 30);
		  const float ry = degreeToRadian<float>(engine->random() % 360);
		  const float rz = degreeToRadian<float>(engine->random() % 30);
		  o.SetRotation(rx, ry, rz);
		  Collision::RigidBodyPtr p(new Collision::BoxShape(trans.ToPosition3F(), o.RotTrans().rot, Vector3F(2.5, 3, 2.5) * scale, scale * scale * scale * (5 * 6 * 5 / 3.0f)));
		  p->thrust = Vector3F(0, 9.8f, 0);
		  engine->pScene->Insert(obj, p, Vector3F(-1.47f, 0.25f, 2.512f) * scale);
		} else {
		  auto obj = engine->renderer.CreateObject("block1", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		  Object& o = *obj;
		  const Vector3F trans(tx, ty + i * unitRegionSize, tz);
		  o.SetTranslation(trans);
		  //		  o.SetScale(Vector3F(2, 2, 2));
		  const float rx = degreeToRadian<float>(engine->random() % 360);
		  const float ry = degreeToRadian<float>(engine->random() % 360);
		  const float rz = degreeToRadian<float>(engine->random() % 360);
		  o.SetRotation(rx, ry, rz);
		  Collision::RigidBodyPtr p(new Collision::BoxShape(trans.ToPosition3F(), o.RotTrans().rot, Vector3F(5, 1, 4) * 2, 10 * 1 * 8));
		  p->thrust = Vector3F(0, 9.8f, 0);
		  engine->pScene->Insert(obj, p, Vector3F(0, 0, -1) * 2);
		}
	  }
	}
#endif
	{
	  auto obj = engine->renderer.CreateObject("ChickenEgg", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  Object& o = *obj;
	  o.SetAnimation(engine->renderer.GetAnimation("Walk"));
	  const Vector3F trans(5, 100, 4.5f);
	  o.SetTranslation(trans);
	  //o.SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(45), degreeToRadian<float>(0));
	  o.SetScale(Vector3F(5, 5, 5));
	  Collision::RigidBodyPtr p(new Collision::SphereShape(trans.ToPosition3F(), 5.0f, 0.1));
	  p->thrust = Vector3F(0, 9.8f, 0);
	  engine->pScene->Insert(obj, p, Vector3F(0, 0, 0));
	}
#if 0
	{
	  auto obj = engine->renderer.CreateObject("Accelerator", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  Object& o = *obj;
	  o.SetScale(Vector3F(5, 5, 5));
	  o.SetTranslation(Vector3F(-15, 120, 0));
	  o.SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(0), degreeToRadian<float>(0));
	  engine->pScene->Insert(obj);
	}
#endif
	{
	  engine->debugObj[0] = engine->renderer.CreateObject("block1", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  Object& o = *engine->debugObj[0];
	  //	  o.SetScale(Vector3F(2, 2, 2));
	  o.SetTranslation(Vector3F(-15, 100, 0));
	  o.SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(0), degreeToRadian<float>(0));
	  engine->pScene->Insert(engine->debugObj[0]);
	}
#if 0
	{
	  auto obj = engine->renderer.CreateObject("FlyingPan", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  Object& o = *obj;
	  o.SetScale(Vector3F(5, 5, 5));
	  o.SetTranslation(Vector3F(15, 10, 0));
	  engine->pScene->Insert(obj);
	}
	{
	  engine->debugObj[1] = engine->renderer.CreateObject("SunnySideUp", Material(Color4B(255, 255, 255, 255), 0.1f, 0), "default");
	  Object& o = *engine->debugObj[1];
	  o.SetScale(Vector3F(3, 3, 3));
	  o.SetTranslation(Vector3F(15, 11, 0));
	  engine->pScene->Insert(engine->debugObj[1]);
	}
#endif
	{
	  auto obj = engine->renderer.CreateObject("ground", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  Object& o = *obj;
	  o.SetRotation(degreeToRadian(-90.0f), 0, 0);
	  Collision::RigidBodyPtr p(new Collision::PlaneShape(Position3F(0, 0, 0), Vector3F(0, 1, 0), 1000 * 1000 * 1000));
	  p->thrust = Vector3F(0, 9.8f, 0);
	  engine->pScene->Insert(obj, p);
	}

	if (0) {
	  auto obj0 = engine->renderer.CreateObject("BrokenEgg", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  Object& o0 = *obj0;
	  o0.SetScale(Vector3F(3, 3, 3));
	  o0.SetTranslation(Vector3F(-12, 0.75f, 0));
	  engine->pScene->Insert(obj0);
	  auto obj2 = engine->renderer.CreateObject("EggYolk", Material(Color4B(255, 255, 255, 255), -0.05f, 0), "default");
	  Object& o2 = *obj2;
	  o2.SetScale(Vector3F(3, 3, 3));
	  o2.SetTranslation(Vector3F(-15.0f, 0.7f, 0.0));
	  engine->pScene->Insert(obj2);
	  auto obj1 = engine->renderer.CreateObject("EggWhite", Material(Color4B(255, 255, 255, 64), -0.5f, 0), "defaultWithAlpha");
	  Object& o1 = *obj1;
	  o1.SetScale(Vector3F(3, 3, 3));
	  o1.SetTranslation(Vector3F(-15, 1, 0));
	  engine->pScene->Insert(obj1);
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
		const int index = boost::random::uniform_int_distribution<>(0, 3)(engine->random);
		const int y = boost::random::uniform_int_distribution<>(heightMin, heightMax)(engine->random);
		int r = boost::random::uniform_int_distribution<>(10, radiusMax)(engine->random);
		//r = radiusMax - r * r / radiusMax2;
		const int a0 = boost::random::uniform_int_distribution<>(0, 359)(engine->random);
		const int a1 = boost::random::uniform_int_distribution<>(30, 60)(engine->random);
		const float scale = boost::random::uniform_int_distribution<>(10, 30)(engine->random) *  0.1f;
		auto obj = engine->renderer.CreateObject(idList[index], Material(Color4B(255, 255, 255, 64), 0, 0), "cloud");
		Object& o = *obj;
		const Quaternion rot0(Vector3F(0, 1, 0), degreeToRadian<float>(a0));
		const Quaternion rot1(Vector3F(0, 1, 0), degreeToRadian<float>(a0));
		const Vector3F trans = rot0.Apply(Vector3F(0, y, r));
		o.SetTranslation(trans);
		o.SetRotation(rot1);
		o.SetScale(Vector3F(scale, scale, scale));
		engine->pScene->Insert(obj);
	  }
	}
#endif

#ifdef SHOW_DEBUG_SENSOR_OBJECT
	engine->debugSensorObj = engine->renderer.CreateObject("octahedron", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
#endif // SHOW_DEBUG_SENSOR_OBJECT

	LOGI("engine_init_display");
	engine->initialized = true;
	return 0;
  }

  /**
  * ディスプレイ内の現在のフレームのみ。
  */
  static void engine_draw_frame(Engine* engine) {
	if (!engine->renderer.HasDisplay() || !engine->initialized) {
	  // ディスプレイがありません。
	  return;
	}

	std::vector<ObjectPtr> objList;
	objList.reserve(10 * 10);
#if 0
	float posY = engine->debugCamera.Position().y;
	if (engine->debugCamera.EyeVector().y >= 0.0f) {
	  posY = std::max(region.min.y, posY - unitRegionSize * 2);
	  for (int i = 0; i < 6; ++i) {
		const Cell& cell = engine->pScene->GetCell(posY);
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
		const Cell& cell = engine->pScene->GetCell(posY);
		for (auto& e : cell.objects) {
		  objList.push_back(e.object);
		}
		posY -= unitRegionSize;
		if (posY < region.min.y) {
		  break;
		}
	  }
	}

	const Position3F eyePos = engine->debugCamera.Position();
	const Vector3F eyeVec = engine->debugCamera.EyeVector();
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
	for (auto itr = engine->pScene->Begin(); itr != engine->pScene->End(); ++itr) {
	  for (auto& e : itr->objects) {
		objList.push_back(e.object);
	  }
	}
#endif

#ifdef SHOW_DEBUG_SENSOR_OBJECT
	{
	  Position3F pos = engine->debugCamera.Position();
	  const Vector3F ev = engine->debugCamera.EyeVector();
	  const Vector3F uv = engine->debugCamera.UpVector();
	  const Vector3F sv = ev.Cross(uv);
	  pos += ev * 4.0f;
	  pos += -uv * 1.5f;
	  pos += sv * 0.9f;
	  engine->debugSensorObj->SetTranslation(Vector3F(pos.x, pos.y, pos.z));

	  // fusedOrientation has each axis range of x(+pi, -pi), y(+pi/2, -pi/2), z(+pi, -pi).
	  const float rx = (engine->fusedOrientation.x + boost::math::constants::pi<float>()) * 0.5f;
	  const float ry = -engine->fusedOrientation.y + boost::math::constants::pi<float>() * 0.5f;
	  const float rz = (engine->fusedOrientation.z + boost::math::constants::pi<float>()) * 0.5f;
	  Matrix4x4 mRot = LookAt(Position3F(0, 0, 0), Position3F(ev.x, ev.y, ev.z), uv);
	  mRot = Matrix4x4::RotationX(rx) * Matrix4x4::RotationX(ry) * Matrix4x4::RotationX(rz) * mRot;
	  Quaternion q;
	  mRot.Decompose(&q, nullptr, nullptr);
	  engine->debugSensorObj->SetRotation(q);
	  objList.push_back(engine->debugSensorObj);
	}
#endif // SHOW_DEBUG_SENSOR_OBJECT

	engine->renderer.ClearDebugString();
	char buf[32];
#if 0
	sprintf(buf, "OX:%1.3f", engine->fusedOrientation.x);
	engine->renderer.AddDebugString(8, 800 - 16 * 9, buf);
	sprintf(buf, "OY:%1.3f", engine->fusedOrientation.y);
	engine->renderer.AddDebugString(8, 800 - 16 * 8, buf);
	sprintf(buf, "OZ:%1.3f", engine->fusedOrientation.z);
	engine->renderer.AddDebugString(8, 800 - 16 * 7, buf);

	sprintf(buf, "MX:%1.3f", engine->magnet.x);
	engine->renderer.AddDebugString(8, 800 - 16 * 6, buf);
	sprintf(buf, "MY:%1.3f", engine->magnet.y);
	engine->renderer.AddDebugString(8, 800 - 16 * 5, buf);
	sprintf(buf, "MZ:%1.3f", engine->magnet.z);
	engine->renderer.AddDebugString(8, 800 - 16 * 4, buf);

	sprintf(buf, "AX:%1.3f", engine->accel.x);
	engine->renderer.AddDebugString(8, 800 - 16 * 3, buf);
	sprintf(buf, "AY:%1.3f", engine->accel.y);
	engine->renderer.AddDebugString(8, 800 - 16 * 2, buf);
	sprintf(buf, "AZ:%1.3f", engine->accel.z);
	engine->renderer.AddDebugString(8, 800 - 16 * 1, buf);
#endif
	sprintf(buf, "FPS:%02d", engine->latestFps);
	engine->renderer.AddDebugString(8, 8, buf);
	sprintf(buf, "AVG:%02.1f", engine->avgFps);
	engine->renderer.AddDebugString(8, 24, buf);
	if (engine->rigidCamera) {
	  sprintf(buf, "%03.1fKm/h", engine->rigidCamera->accel.Length() * 3.6f);
	  engine->renderer.AddDebugString(8, 40, buf);
	}

	engine->renderer.Render(&objList[0], &objList[0] + objList.size());
	engine->renderer.Swap();
  }


  /**
  * 現在ディスプレイに関連付けられている EGL コンテキストを削除します。
  */
  static void engine_term_display(Engine* engine) {
	engine->initialized = false;
	engine->renderer.Unload();
	engine->pScene->Clear();
	LOGI("engine_term_display");
  }
#if 0
  /**
  * 次の入力イベントを処理します。
  */
  static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* engine = (struct engine*)app->userData;
	return engine->debugCamera.HandleEvent(event);
  }

  /**
  * 次のメイン コマンドを処理します。
  */
  static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = (struct engine*)app->userData;
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
	  // 現在の状態を保存するようシステムによって要求されました。保存してください。
	  engine->app->savedState = malloc(sizeof(struct saved_state));
	  *((struct saved_state*)engine->app->savedState) = engine->state;
	  engine->app->savedStateSize = sizeof(struct saved_state);
	  break;
	case APP_CMD_INIT_WINDOW:
	  // ウィンドウが表示されています。準備してください。
	  if (engine->app->window != NULL) {
		engine_init_display(engine);
		engine_draw_frame(engine);
	  }
	  break;
	case APP_CMD_TERM_WINDOW:
	  // ウィンドウが非表示または閉じています。クリーン アップしてください。
	  engine_term_display(engine);
	  break;
	case APP_CMD_GAINED_FOCUS:
	  // アプリがフォーカスを取得すると、加速度計の監視を開始します。
	  for (int i = 0; i < engine::SensorType_MAX; ++i) {
		if (engine->sensor[i]) {
		  ASensorEventQueue_enableSensor(engine->sensorEventQueue[i], engine->sensor[i]);
		  // 目標は 1 秒ごとに 60 のイベントを取得することです (米国)。
		  ASensorEventQueue_setEventRate(engine->sensorEventQueue[i], engine->sensor[i], (1000L / 60) * 1000);
		}
	  }
	  break;
	case APP_CMD_LOST_FOCUS:
	  // アプリがフォーカスを失うと、加速度計の監視を停止します。
	  // これにより、使用していないときのバッテリーを節約できます。
	  for (int i = 0; i < engine::SensorType_MAX; ++i) {
		if (engine->sensor[i]) {
		  ASensorEventQueue_disableSensor(engine->sensorEventQueue[i], engine->sensor[i]);
		}
	  }
	  engine_draw_frame(engine);
	  break;
	}
  }
#endif

  int_fast64_t get_time() {
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
  }

  float time_to_sec(int_fast64_t time) { return static_cast<float>(static_cast<double>(time) / (1000 * 1000 * 1000)); }

  struct Camera {
	Camera(const Mai::Position3F& pos, const Mai::Vector3F& at, const Mai::Vector3F& up)
	  : position(pos)
	  , eyeVector(at)
	  , upVector(up)
	{}

	Position3F position;
	Vector3F eyeVector;
	Vector3F upVector;
  };

  void run(android_app* app) {
	Engine engine(app);
	engine.window.Initialize("Sunny Side Up", 480, 800);
	engine.window.SetVisible(true);

	FileSystem::Initialize(app->activity->assetManager);

	Camera camera(Mai::Position3F(0, 0, 0), Mai::Vector3F(0, 0, -1), Mai::Vector3F(0, 1, 0));
	int mouseX = -1, mouseY = -1;
	while (1) {
	  engine.window.MessageLoop();
	  engine.window.CalcFusedOrientation();
	  while (auto e = engine.window.PopEvent()) {
		switch (e->Type) {
		case Event::EVENT_CLOSED:
		  engine_term_display(&engine);
		  return;
		case Event::EVENT_INIT_WINDOW:
		  engine_init_display(&engine);
		  break;
		case Event::EVENT_TERM_WINDOW:
		  engine_term_display(&engine);
		  break;
		case Event::EVENT_MOUSE_MOVED:
		  if (mouseX >= 0) {
			const float x = static_cast<float>(mouseX - e->MouseMove.X) * 0.005f;
			const float y = static_cast<float>(mouseY - e->MouseMove.Y) * 0.005f;
			const Mai::Vector3F leftVector = camera.eyeVector.Cross(camera.upVector).Normalize();
			camera.eyeVector = (Mai::Quaternion(camera.upVector, x) * Mai::Quaternion(leftVector, y)).Apply(camera.eyeVector).Normalize();
			camera.upVector = Mai::Quaternion(leftVector, y).Apply(camera.upVector).Normalize();
		  }
		  mouseX = e->MouseMove.X;
		  mouseY = e->MouseMove.Y;
		  break;
		default:
		  break;
		}
	  }

	  if (engine.initialized) {
		// イベントが完了したら次のアニメーション フレームを描画します。
#if 1
		++engine.frames;
		const int64_t curTime = get_time();
		if (curTime < engine.startTime) {
		  engine.startTime = curTime;
		  engine.frames = 0;
		} else if (curTime - engine.startTime >= (1000UL * 1000UL * 1000UL)) {
		  engine.latestFps = std::min(engine.frames, 60);
		  engine.startTime = curTime;
		  engine.frames = 0;
		  if (engine.latestFps > engine.avgFps * 0.3f) { // 低すぎるFPSはノイズとみなす.
			engine.avgFps = (engine.avgFps + engine.latestFps) * 0.5f;
			engine.deltaTime = (engine.deltaTime + 1.0f / engine.latestFps) * 0.5f;
		  }
		}
#endif
#if 1
		//	  engine.debugCamera.Update(engine.deltaTime, engine.fusedOrientation);
		engine.pScene->Update(engine.deltaTime);
#else
		const Position3F prevCamPos = engine.debugCamera.Position();
		engine.debugCamera.Update(engine.deltaTime, engine.fusedOrientation);
		Vector3F camVec = engine.debugCamera.Position() - prevCamPos;
		if (engine.rigidCamera) {
		  if (camVec.LengthSq() == 0.0f) {
			Vector3F v = engine.rigidCamera->accel;
			v.y = 0;
			if (v.LengthSq() < 0.1f) {
			  v = Vector3F::Unit();
			} else {
			  v.Normalize();
			  v *= 0.1f;
			}
			engine.rigidCamera->accel -= v;
		  } else {
			engine.rigidCamera->accel = camVec * 10.0f;
		  }
		  if (engine.rigidCamera->Position().y < 5.0f && engine.debugCamera.Direction() == TouchSwipeCamera::MoveDir_Back) {
			engine.debugCamera.Direction(TouchSwipeCamera::MoveDir_Newtral);
			engine.rigidCamera->Position(Position3F(0, 4000, 0));
		  }
		}
		engine.pScene->Update(engine.deltaTime);
		if (engine.rigidCamera) {
		  Position3F pos = engine.rigidCamera->Position();
		  pos.y += 10.0f;
		  engine.debugCamera.Position(pos);
		}
#endif
		engine.renderer.Update(engine.deltaTime, camera.position, camera.eyeVector, camera.upVector);
		//			for (auto&& e : engine.obj) {
		//				e.Update(engine.deltaTime);
		//				while (e.GetCurrentTime() >= 1.0f) {
		//					e.SetCurrentTime(e.GetCurrentTime() - 1.0f);
		//				}
		//			}

		if (engine.debugObj[0]) {
		  Object& o = *engine.debugObj[0];
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
		if (engine.debugObj[1]) {
		  Object& o = *engine.debugObj[1];
		  static float roughness = 0, metallic = 0;
		  o.SetRoughness(roughness);
		  o.SetMetallic(metallic);
		}
#if 0
		static float roughness = 1.0f;
		static float step = 0.005;
		engine.obj[1].SetRoughness(roughness);
		roughness += step;
		if (roughness >= 1.0) {
		  step = -0.005f;
		} else if (roughness <= 0.0) {
		  step = 0.005f;
		}
#endif

		// 描画は画面の更新レートに合わせて調整されているため、
		// ここで時間調整をする必要はありません。
		engine_draw_frame(&engine);
	  }
	}
  }
} // namespace Mai

/**
* これは、android_native_app_glue を使用しているネイティブ アプリケーション
* のメイン エントリ ポイントです。それ自体のスレッドでイベント ループによって実行され、
* 入力イベントを受け取ったり他の操作を実行したりします。
*/
void android_main(android_app* app) {
  Mai::run(app);
}
