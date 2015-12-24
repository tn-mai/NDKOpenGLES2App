/*
* Copyright (C) 2010 The Android Open Source Project
*
* Apache License Version 2.0 (�u�{���C�Z���X�v) �Ɋ�Â��ă��C�Z���X����܂��B;
* �{���C�Z���X�ɏ������Ȃ��ꍇ�͂��̃t�@�C�����g�p�ł��܂���B
* �{���C�Z���X�̃R�s�[�́A�ȉ��̏ꏊ�������ł��܂��B
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* �K�p�����@�߂܂��͏��ʂł̓��ӂɂ���Ė������Ȃ�����A�{���C�Z���X�Ɋ�Â��ĔЕz�����\�t�g�E�F�A�́A
* �����َ����킸�A�����Ȃ�ۏ؂��������Ȃ��Ɍ���̂܂�
* �Еz����܂��B
* �{���C�Z���X�ł̌����Ɛ������K�肵�������ɂ��ẮA
* �{���C�Z���X���Q�Ƃ��Ă��������B
*
*/

#include "Scene.h"
#include "Renderer.h"
#include "android_native_app_glue.h"
#include "TouchSwipeCamera.h"
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

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))

//#define SHOW_DEBUG_SENSOR_OBJECT

static const Region region = { Position3F(-100, 200, -100), Position3F(100, 3800, 100) };
static const float unitRegionSize = 100.0f;
static const size_t maxObject = 1000;

/**
* �ۑ���Ԃ̃f�[�^�ł��B
*/
struct saved_state {
	float angle;
	int32_t x;
	int32_t y;
	saved_state() : angle(0), x(0), y(0) {}
};

/**
* �A�v���̕ۑ���Ԃł��B
*
* A sensor managiment code was referring to http://plaw.info/2012/03/android-sensor-fusion-tutorial/
*/
struct engine {
	engine(android_app* a)
		: app(a)
		, sensorManager(0)
		, animating(0)
		, initialized(false)
//		, display(0)
//		, surface(0)
//		, context(0)
//		, width(0)
//		, height(0)
		, state()
	    , debugCamera(a)
		, renderer(a)
		, pScene(new Scene(region.min, region.max, unitRegionSize, maxObject))
		, random(time(nullptr))
		, avgFps(30.0f)
		, deltaTime(1.0f / avgFps)
		, frames(0)
		, latestFps(30)
		, startTime(0)
	{
		for (int i = 0; i < SensorType_MAX; ++i) {
			sensor[i] = 0;
			sensorEventQueue[i] = 0;
		}
		gyro = Vector3F::Unit();
		gyroMatrix = Matrix4x4::Unit();
		gyroOrientation = Vector3F::Unit();
		magnet = Vector3F::Unit();
		accel = Vector3F::Unit();
		accMagOrientation = Vector3F::Unit();
		fusedOrientation = Vector3F::Unit();
	}

	struct android_app* app;

	enum SensorType {
	  SensorType_Accel,
	  SensorType_Magnet,
	  SensorType_Gyro,
	  SensorType_MAX
	};
	enum LooperId {
	  LooperId_Accel = LOOPER_ID_USER,
	  LooperId_Magnet,
	  LooperId_Gyro
	};
	ASensorManager* sensorManager;
	const ASensor* sensor[SensorType_MAX];
	ASensorEventQueue* sensorEventQueue[SensorType_MAX];
	Vector3F gyro;
	Matrix4x4 gyroMatrix;
	Vector3F gyroOrientation;
	Vector3F magnet;
	Vector3F accel;
	Vector3F accMagOrientation;
	Vector3F fusedOrientation;

	int animating;
	bool initialized;
//	EGLDisplay display;
//	EGLSurface surface;
//	EGLContext context;
//	int32_t width;
//	int32_t height;
	struct saved_state state;

	TouchSwipeCamera debugCamera;
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

	static bool GetRotationMatrix(Matrix4x4* m, const Vector3F& gravity, const Vector3F& geomagnetic) {
		Vector3F A(gravity);
		const Vector3F E(geomagnetic);
		Vector3F H = E.Cross(A);
		const float normH = H.Length();
		if (normH < 0.1f) {
			return false;
		}
		H.Normalize();
		A.Normalize();
		const Vector3F M = A.Cross(H);

		m->SetVector(0, Vector4F(H, 0));
		m->SetVector(1, Vector4F(M, 0));
		m->SetVector(2, Vector4F(A, 0));
		m->SetVector(3, Vector4F::Unit());
		return true;
	}

	static void GetOrientation(Vector3F* v, const Matrix4x4& m) {
	  v->x = std::atan2(m.At(0, 1), m.At(1, 1));
	  v->y = std::asin(-m.At(2, 1));
	  v->z = std::atan2(-m.At(2, 0), m.At(2, 2));
	}

	void CalcAccMagOrientation() {
		Matrix4x4 m;
		if (GetRotationMatrix(&m, accel, magnet)) {
			GetOrientation(&accMagOrientation, m);
		} else {
			accMagOrientation = accel;
		}
	}

	void GetQuaternionFromGyro(Quaternion* deltaRotationVector, Vector3F gyro, float timeFactor) {
		const float omegaMagnitude = gyro.Length();
		const float epsilon = 0.000000001f;
		if (omegaMagnitude > epsilon) {
		  gyro.Normalize();
		}
		*deltaRotationVector = Quaternion(gyro, omegaMagnitude *  timeFactor);
	}

	static void GetRotationMatrixFromOrientation(Matrix4x4* m, const Vector3F& o) {
		*m = Matrix4x4::RotationX(o.x) * Matrix4x4::RotationY(o.y) * Matrix4x4::RotationZ(o.z);
	}
	
	static void GetRotationMatrixFromQuaternion(Matrix4x4* m, const Quaternion& r) {
		const Quaternion q(r.x, r.y, r.z, std::sqrt(1.0f - r.Length()));
		const Matrix4x3 mm = ToMatrix(q);
		m->SetVector(0, mm.GetVector(0));
		m->SetVector(1, mm.GetVector(1));
		m->SetVector(2, mm.GetVector(2));
		m->SetVector(0, Vector4F::Unit());
	}

	void GyroFunction(const Vector3F& vector, float timestamp) {
		static bool initState = true;
		static float timeStamp = 0.0f;
		if (initState) {
			Matrix4x4 m;
			GetRotationMatrixFromOrientation(&m, accMagOrientation);
			gyroMatrix *= m;
			initState = false;
		}
		Quaternion delta = Quaternion::Unit();
		if (timeStamp != 0.0f) {
			const float NS2S = 1.0f / 1000000000.0f;
			const float dt = (timestamp - timeStamp) * NS2S;
			gyro = Vector3F(vector.x, vector.y, vector.z);
			GetQuaternionFromGyro(&delta, gyro, dt);
		}
		timeStamp = timestamp;
		Matrix4x4 deltaMatrix;
		GetRotationMatrixFromQuaternion(&deltaMatrix, delta);
		gyroMatrix *= deltaMatrix;
		GetOrientation(&gyroOrientation, gyroMatrix);
	}

	void CalcFusedOrientation() {
		const float coeff = 0.98f;
		const float oneMinusCoeff = 1.0f - coeff;
		if (sensor[SensorType_Gyro]) {
			fusedOrientation = gyroOrientation * coeff + accMagOrientation * oneMinusCoeff;
			GetRotationMatrixFromOrientation(&gyroMatrix, fusedOrientation);
		} else if (sensor[SensorType_Magnet]) {
			fusedOrientation = accMagOrientation;
		} else {
		  /* 003SH�̓���d�l����K���ɐݒ�.
		  accel: �f�o�C�X��up�x�N�g��.
		  �f�o�C�X������ x=������(�E��+), y=������(�オ+), z=�O�㎲(�O��+).
		  ����������̉�]�����o�ł��Ȃ�.
		  */
		  fusedOrientation.x = 0.0f;
		  fusedOrientation.y = -accel.y * 0.1f * boost::math::constants::pi<float>() * 0.5f;
		  fusedOrientation.z = std::atan(accel.z / accel.x);
		  if (fusedOrientation.z >= 0.0f) {
			fusedOrientation.z -= boost::math::constants::pi<float>() * 0.5f;
		  } else {
			fusedOrientation.z += boost::math::constants::pi<float>() * 0.5f;
		  }
		}
	}
};

template<typename T>
T degreeToRadian(T d) {
  return d * boost::math::constants::pi<T>() / static_cast<T>(180);
}

/**
* ���݂̕\���� EGL �R���e�L�X�g�����������܂��B
*/
static int engine_init_display(struct engine* engine) {
	// OpenGL ES �� EGL �̏�����
#if 0
	/*4
	* �ړI�̍\���̑����������Ŏw�肵�܂��B
	* �ȉ��ŁA�I���X�N���[�� �E�B���h�E��
	* �݊����̂���A�e�F�Œ� 8 �r�b�g�̃R���|�[�l���g�� EGLConfig ��I�����܂�
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

	/* �����ŁA�A�v���P�[�V�����͖ړI�̍\����I�����܂��B���̃T���v���ł́A
	* ���o�����ƈ�v����ŏ��� EGLConfig ��
	* �I������P���ȑI���v���Z�X������܂� */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID �́AANativeWindow_setBuffersGeometry() ��
	* ����Ď󂯎���邱�Ƃ��ۏ؂���Ă��� EGLConfig �̑����ł��B
	* EGLConfig ��I�������炷���ɁAANativeWindow �o�b�t�@�[����v�����邽�߂�
	* EGL_NATIVE_VISUAL_ID ���g�p���Ĉ��S�ɍč\���ł��܂��B*/
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
	// GL �̏�Ԃ����������܂��B
	engine->renderer.Initialize();
	engine->debugCamera.Reset();
	engine->debugCamera.ScreenSize(engine->renderer.Width(), engine->renderer.Height());
	engine->debugCamera.Position(Position3F(0, 100, 0));
	engine->debugCamera.LookAt(Position3F(-1, 100, 0));
	engine->debugCamera.UpVector(Vector3F(0, 1, 0));
#if 0
	// �����_���ɃI�u�W�F�N�g�𔭐������Ă݂�.
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
* �f�B�X�v���C���̌��݂̃t���[���̂݁B
*/

static void engine_draw_frame(struct engine* engine) {
  if (!engine->renderer.HasDisplay() || !engine->initialized) {
	// �f�B�X�v���C������܂���B
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
* ���݃f�B�X�v���C�Ɋ֘A�t�����Ă��� EGL �R���e�L�X�g���폜���܂��B
*/
static void engine_term_display(struct engine* engine) {
	engine->initialized = false;
	engine->renderer.Unload();
	engine->pScene->Clear();
	LOGI("engine_term_display");
}

/**
* ���̓��̓C�x���g���������܂��B
*/
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* engine = (struct engine*)app->userData;
	return engine->debugCamera.HandleEvent(event);
}

/**
* ���̃��C�� �R�}���h���������܂��B
*/
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = (struct engine*)app->userData;
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
		// ���݂̏�Ԃ�ۑ�����悤�V�X�e���ɂ���ėv������܂����B�ۑ����Ă��������B
		engine->app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)engine->app->savedState) = engine->state;
		engine->app->savedStateSize = sizeof(struct saved_state);
		break;
	case APP_CMD_INIT_WINDOW:
		// �E�B���h�E���\������Ă��܂��B�������Ă��������B
		if (engine->app->window != NULL) {
			engine_init_display(engine);
			engine_draw_frame(engine);
		}
		break;
	case APP_CMD_TERM_WINDOW:
		// �E�B���h�E����\���܂��͕��Ă��܂��B�N���[�� �A�b�v���Ă��������B
		engine_term_display(engine);
		break;
	case APP_CMD_GAINED_FOCUS:
		// �A�v�����t�H�[�J�X���擾����ƁA�����x�v�̊Ď����J�n���܂��B
		for (int i = 0; i < engine::SensorType_MAX; ++i) {
			if (engine->sensor[i]) {
				ASensorEventQueue_enableSensor(engine->sensorEventQueue[i], engine->sensor[i]);
				// �ڕW�� 1 �b���Ƃ� 60 �̃C�x���g���擾���邱�Ƃł� (�č�)�B
				ASensorEventQueue_setEventRate(engine->sensorEventQueue[i], engine->sensor[i], (1000L / 60) * 1000);
			}
		}
		break;
	case APP_CMD_LOST_FOCUS:
		// �A�v�����t�H�[�J�X�������ƁA�����x�v�̊Ď����~���܂��B
		// ����ɂ��A�g�p���Ă��Ȃ��Ƃ��̃o�b�e���[��ߖ�ł��܂��B
		for (int i = 0; i < engine::SensorType_MAX; ++i) {
			if (engine->sensor[i]) {
				ASensorEventQueue_disableSensor(engine->sensorEventQueue[i], engine->sensor[i]);
			}
		}
		engine_draw_frame(engine);
		break;
	}
}

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

float time_to_sec(int_fast64_t time) { return static_cast<float>(static_cast<double>(time) / (1000 * 1000 * 1000));  }

/**
* ����́Aandroid_native_app_glue ���g�p���Ă���l�C�e�B�u �A�v���P�[�V����
* �̃��C�� �G���g�� �|�C���g�ł��B���ꎩ�̂̃X���b�h�ŃC�x���g ���[�v�ɂ���Ď��s����A
* ���̓C�x���g���󂯎�����葼�̑�������s�����肵�܂��B
*/
void android_main(struct android_app* state) {
	struct engine engine(state);

	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;

	ANativeActivity_setWindowFlags(state->activity, AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

	// �Z���T�[�̊Ď��̏���
	static const int sensorTypeList[] = {
		ASENSOR_TYPE_ACCELEROMETER,
		ASENSOR_TYPE_MAGNETIC_FIELD,
		ASENSOR_TYPE_GYROSCOPE,
	};
	engine.sensorManager = ASensorManager_getInstance();
	for (int i = engine::SensorType_Accel; i < engine::SensorType_MAX; ++i) {
		engine.sensor[i] = ASensorManager_getDefaultSensor(engine.sensorManager, sensorTypeList[i]);
		if (engine.sensor[i]) {
			engine.sensorEventQueue[i] = ASensorManager_createEventQueue(engine.sensorManager, state->looper, LOOPER_ID_USER + i, NULL, NULL);
		}
	}
	if (state->savedState != NULL) {
		// �ȑO�̕ۑ���ԂŊJ�n���܂��B�������Ă��������B
		engine.state = *(struct saved_state*)state->savedState;
	}

	engine.animating = 1;

	// ���[�v�̓X�^�b�t�ɂ��J�n��҂��Ă��܂��B

	while (1) {
		// �ۗ����̂��ׂẴC�x���g��ǂݎ��܂��B
		int ident;
		int events;
		struct android_poll_source* source;

		// �A�j���[�V�������Ȃ��ꍇ�A�������Ƀu���b�N���ăC�x���g����������̂�҂��܂��B
		// �A�j���[�V��������ꍇ�A���ׂẴC�x���g���ǂݎ����܂Ń��[�v���Ă��瑱�s���܂�
		// �A�j���[�V�����̎��̃t���[����`�悵�܂��B
		while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events, (void**)&source)) >= 0) {

			// ���̃C�x���g���������܂��B
			if (source != NULL) {
				source->process(state, source);
			}

			// �Z���T�[�Ƀf�[�^������ꍇ�A�������������܂��B
			switch (ident) {
			case engine::LooperId_Accel:
				if (engine.sensor[ident - LOOPER_ID_USER]) {
					ASensorEvent event;
					float timestamp = 0.0f;
					while (ASensorEventQueue_getEvents(engine.sensorEventQueue[ident - LOOPER_ID_USER], &event, 1) > 0) {
						if (timestamp == 0.0f) {
							timestamp = event.timestamp;
						}
						engine.accel.x = event.acceleration.x;
						engine.accel.y = event.acceleration.y;
						engine.accel.z = event.acceleration.z;
						engine.CalcAccMagOrientation();
						break;
					}
				}
				break;
			case engine::LooperId_Magnet:
				if (engine.sensor[ident - LOOPER_ID_USER]) {
					ASensorEvent event;
					while (ASensorEventQueue_getEvents(engine.sensorEventQueue[ident - LOOPER_ID_USER], &event, 1) > 0) {
						engine.magnet.x = event.magnetic.x;
						engine.magnet.y = event.magnetic.y;
						engine.magnet.z = event.magnetic.z;
						break;
					}
				}
				break;
			case engine::LooperId_Gyro:
				if (engine.sensor[ident - LOOPER_ID_USER]) {
					ASensorEvent event;
					while (ASensorEventQueue_getEvents(engine.sensorEventQueue[ident - LOOPER_ID_USER], &event, 1) > 0) {
						engine.GyroFunction(Vector3F(event.vector.x, event.vector.y, event.vector.z), event.timestamp);
						break;
					}
				}
				break;
			};

			// �I�����邩�ǂ����m�F���܂��B
			if (state->destroyRequested != 0) {
				engine_term_display(&engine);
				return;
			}
		}
		engine.CalcFusedOrientation();

		if (engine.animating) {
			// �C�x���g�����������玟�̃A�j���[�V���� �t���[����`�悵�܂��B
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
			  if (engine.latestFps > engine.avgFps * 0.3f) { // �Ⴗ����FPS�̓m�C�Y�Ƃ݂Ȃ�.
				engine.avgFps = (engine.avgFps + engine.latestFps) * 0.5f;
				engine.deltaTime = (engine.deltaTime + 1.0f / engine.latestFps) * 0.5f;
			  }
			}
#endif
#if 1
			engine.debugCamera.Update(engine.deltaTime, engine.fusedOrientation);
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
			engine.renderer.Update(engine.deltaTime, engine.debugCamera.Position(), engine.debugCamera.EyeVector(), engine.debugCamera.UpVector());
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

			// �`��͉�ʂ̍X�V���[�g�ɍ��킹�Ē�������Ă��邽�߁A
			// �����Ŏ��Ԓ���������K�v�͂���܂���B
			engine_draw_frame(&engine);
		}
	}
}
