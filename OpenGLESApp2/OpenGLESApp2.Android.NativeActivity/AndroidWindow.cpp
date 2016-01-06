#include "AndroidWindow.h"
#include "../../Common/Quaternion.h"
#include <android/window.h>
#include <android/sensor.h>
#include <boost/math/constants/constants.hpp>

namespace Mai {
  namespace {
	/**
	* 次の入力イベントを処理します。
	*/
	int32_t InputHandler(struct android_app* app, AInputEvent* event) {
	  AndroidWindow* window = reinterpret_cast<AndroidWindow*>(app->userData);
	  return window->InputHandler(event);
	}

	/**
	* 次のメイン コマンドを処理します。
	*/
	void CommandHandler(struct android_app* app, int32_t cmd) {
	  AndroidWindow* window = reinterpret_cast<AndroidWindow*>(app->userData);
	  return window->CommandHandler(cmd);
	}

	bool GetRotationMatrix(Matrix4x4* m, const Vector3F& gravity, const Vector3F& geomagnetic) {
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

	void GetOrientation(Vector3F* v, const Matrix4x4& m) {
	  v->x = std::atan2(m.At(0, 1), m.At(1, 1));
	  v->y = std::asin(-m.At(2, 1));
	  v->z = std::atan2(-m.At(2, 0), m.At(2, 2));
	}

	void CalcAccMagOrientation(Vector3F* v, const Vector3F& accel, const Vector3F& magnet) {
	  Matrix4x4 m;
	  if (GetRotationMatrix(&m, accel, magnet)) {
		GetOrientation(v, m);
	  } else {
		*v = accel;
	  }
	}

	void GetRotationMatrixFromOrientation(Matrix4x4* m, const Vector3F& o) {
	  *m = Matrix4x4::RotationX(o.x) * Matrix4x4::RotationY(o.y) * Matrix4x4::RotationZ(o.z);
	}

	void GetRotationMatrixFromQuaternion(Matrix4x4* m, const Quaternion& r) {
	  const Quaternion q(r.x, r.y, r.z, std::sqrt(1.0f - r.Length()));
	  const Matrix4x3 mm = ToMatrix(q);
	  m->SetVector(0, mm.GetVector(0));
	  m->SetVector(1, mm.GetVector(1));
	  m->SetVector(2, mm.GetVector(2));
	  m->SetVector(0, Vector4F::Unit());
	}

	void GetQuaternionFromGyro(Quaternion* deltaRotationVector, Vector3F gyro, float timeFactor) {
	  const float omegaMagnitude = gyro.Length();
	  const float epsilon = 0.000000001f;
	  if (omegaMagnitude > epsilon) {
		gyro.Normalize();
	  }
	  *deltaRotationVector = Quaternion(gyro, omegaMagnitude *  timeFactor);
	}

  } // unnamed namespace

  AndroidWindow::AndroidWindow(android_app* p)
	: app(p)
	, suspending(false)
	, sensorManager(0)
	, savedState()
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

	touchSwipeState.dpFactor = 160.0f / AConfiguration_getDensity(app->config);
	touchSwipeState.dragging = false;
  }

  EGLNativeWindowType AndroidWindow::GetWindowType() const {
	return app->window;
  }

  EGLNativeDisplayType AndroidWindow::GetDisplayType() const {
	return 0;
  }

  void AndroidWindow::MessageLoop() {
	int ident;
	int events;
	android_poll_source* source;
	while ((ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) {
	  if (source != NULL) {
		source->process(app, source);
	  }

	  switch (ident) {
	  case LooperId_Accel:
		if (sensor[ident - LOOPER_ID_USER]) {
		  ASensorEvent event;
		  float timestamp = 0.0f;
		  while (ASensorEventQueue_getEvents(sensorEventQueue[ident - LOOPER_ID_USER], &event, 1) > 0) {
			if (timestamp == 0.0f) {
			  timestamp = event.timestamp;
			}
			accel.x = event.acceleration.x;
			accel.y = event.acceleration.y;
			accel.z = event.acceleration.z;
			CalcAccMagOrientation(&accMagOrientation, accel, magnet);
			break;
		  }
		}
		break;
	  case LooperId_Magnet:
		if (sensor[ident - LOOPER_ID_USER]) {
		  ASensorEvent event;
		  while (ASensorEventQueue_getEvents(sensorEventQueue[ident - LOOPER_ID_USER], &event, 1) > 0) {
			magnet.x = event.magnetic.x;
			magnet.y = event.magnetic.y;
			magnet.z = event.magnetic.z;
			break;
		  }
		}
		break;
	  case LooperId_Gyro:
		if (sensor[ident - LOOPER_ID_USER]) {
		  ASensorEvent event;
		  while (ASensorEventQueue_getEvents(sensorEventQueue[ident - LOOPER_ID_USER], &event, 1) > 0) {
			GyroFunction(Vector3F(event.vector.x, event.vector.y, event.vector.z), event.timestamp);
			break;
		  }
		}
		break;
	  };

	  // 終了するかどうか確認します。
	  if (app->destroyRequested != 0) {
		Event event;
		event.Type = Event::EVENT_CLOSED;
		PushEvent(event);
		return;
	  }
	}
  }

  bool AndroidWindow::Initialize(const char* name, size_t width, size_t height) {
	app->userData = this;
	app->onAppCmd = ::Mai::CommandHandler;
	app->onInputEvent = ::Mai::InputHandler;

	ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

	// センサーの監視の準備
	static const int sensorTypeList[] = {
	  ASENSOR_TYPE_ACCELEROMETER,
	  ASENSOR_TYPE_MAGNETIC_FIELD,
	  ASENSOR_TYPE_GYROSCOPE,
	};
	sensorManager = ASensorManager_getInstance();
	for (int i = SensorType_Accel; i < SensorType_MAX; ++i) {
	  sensor[i] = ASensorManager_getDefaultSensor(sensorManager, sensorTypeList[i]);
	  if (sensor[i]) {
		sensorEventQueue[i] = ASensorManager_createEventQueue(sensorManager, app->looper, LOOPER_ID_USER + i, NULL, NULL);
	  }
	}

	LoadState();

	return true;
  }

  void AndroidWindow::Destroy() {
  }

  void AndroidWindow::CalcFusedOrientation() {
	const float coeff = 0.98f;
	const float oneMinusCoeff = 1.0f - coeff;
	if (sensor[SensorType_Gyro]) {
	  fusedOrientation = gyroOrientation * coeff + accMagOrientation * oneMinusCoeff;
	  GetRotationMatrixFromOrientation(&gyroMatrix, fusedOrientation);
	} else if (sensor[SensorType_Magnet]) {
	  fusedOrientation = accMagOrientation;
	} else {
	  /* 003SHの動作仕様から適当に設定.
	  accel: デバイスのupベクトル.
	  デバイス垂直時 x=水平軸(右が+), y=垂直軸(上が+), z=前後軸(前が+).
	  垂直軸周りの回転を検出できない.
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

  void AndroidWindow::CommandHandler(int32_t cmd) {
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
	  // 現在の状態を保存するようシステムによって要求されました。保存してください。
	  SaveState();
	  break;
	case APP_CMD_INIT_WINDOW:
	  // ウィンドウが表示されています。準備してください。
	  if (app->window != NULL) {
		Event event;
		event.Type = Event::EVENT_INIT_WINDOW;
		PushEvent(event);
	  }
	  break;
	case APP_CMD_TERM_WINDOW: {
	  // ウィンドウが非表示または閉じています。クリーン アップしてください。
	  Event event;
	  event.Type = Event::EVENT_TERM_WINDOW;
	  PushEvent(event);
	  break;
	}
	case APP_CMD_GAINED_FOCUS:
	  // アプリがフォーカスを取得すると、加速度計の監視を開始します。
	  for (int i = 0; i < SensorType_MAX; ++i) {
		if (sensor[i]) {
		  ASensorEventQueue_enableSensor(sensorEventQueue[i], sensor[i]);
		  // 目標は 1 秒ごとに 60 のイベントを取得することです (米国)。
		  ASensorEventQueue_setEventRate(sensorEventQueue[i], sensor[i], (1000L / 60) * 1000);
		}
	  }
	  break;
	case APP_CMD_LOST_FOCUS:
	  // アプリがフォーカスを失うと、加速度計の監視を停止します。
	  // これにより、使用していないときのバッテリーを節約できます。
	  for (int i = 0; i < SensorType_MAX; ++i) {
		if (sensor[i]) {
		  ASensorEventQueue_disableSensor(sensorEventQueue[i], sensor[i]);
		}
	  }
	  break;
	}
  }

  bool AndroidWindow::InputHandler(AInputEvent* event) {
	return TouchSwipeHandler(event);
  }

  bool AndroidWindow::TouchSwipeHandler(AInputEvent* event)
  {
	if (suspending) {
	  return false;
	}
	if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
	  return false;
	}
	const size_t count = AMotionEvent_getPointerCount(event);
	const int32_t actionAndIndex = AMotionEvent_getAction(event);
	const int32_t action = actionAndIndex & AMOTION_EVENT_ACTION_MASK;
	const int32_t index = (actionAndIndex & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
	bool consumed = false;
	if (count == 1) {
	  switch (action) {
	  case AMOTION_EVENT_ACTION_DOWN:
		touchSwipeState.tapInfo.id = AMotionEvent_getPointerId(event, 0);
		touchSwipeState.tapInfo.pos = Vector2F(AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));
		consumed = true;
		break;
	  case AMOTION_EVENT_ACTION_UP: {
		const int32_t TAP_TIMEOUT = 180 * 1000000;
		const int32_t TOUCH_SLOP = 8;
		const int64_t eventTime = AMotionEvent_getEventTime(event);
		const int64_t downTime = AMotionEvent_getDownTime(event);
		if (eventTime - downTime <= TAP_TIMEOUT) {
		  if (touchSwipeState.tapInfo.id == AMotionEvent_getPointerId(event, 0)) {
			const Vector2F curPos(AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));
			if ((curPos - touchSwipeState.tapInfo.pos).Length() < TOUCH_SLOP * touchSwipeState.dpFactor) {
			  Event e;
			  e.Type = Event::EVENT_MOUSE_BUTTON_PRESSED;
			  e.MouseButton.Button = MOUSEBUTTON_LEFT;
			  e.MouseButton.X = curPos.x;
			  e.MouseButton.Y = curPos.y;
			  PushEvent(e);
			  consumed = true;
			}
		  }
		}
		break;
	  }
	  }
	}

	{
	  switch (action) {
	  case AMOTION_EVENT_ACTION_DOWN: {
		touchSwipeState.dragInfo.id = AMotionEvent_getPointerId(event, 0);
		touchSwipeState.dragInfo.pos = Vector2F(AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));
		touchSwipeState.dragging = true;
		consumed = true;
		return true;
	  }
	  case AMOTION_EVENT_ACTION_POINTER_DOWN:
		break;
	  case AMOTION_EVENT_ACTION_UP:
		touchSwipeState.dragging = false;
		consumed = true;
		return true;
	  case AMOTION_EVENT_ACTION_POINTER_UP: {
		const int id = AMotionEvent_getPointerId(event, index);
		if (touchSwipeState.dragInfo.id == id) {
		  touchSwipeState.dragging = false;
		  consumed = true;
		}
		break;
	  }
	  case AMOTION_EVENT_ACTION_MOVE:
		if (touchSwipeState.dragging) {
		  touchSwipeState.dragInfo.pos = Vector2F(AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));
		  Event e;
		  e.Type = Event::EVENT_MOUSE_MOVED;
		  e.MouseMove.X = touchSwipeState.dragInfo.pos.x;
		  e.MouseMove.Y = touchSwipeState.dragInfo.pos.y;
		  PushEvent(e);
		  consumed = true;
		}
		break;
	  }
	}
	return consumed;
  }

  void AndroidWindow::SaveState() {
	app->savedState = malloc(sizeof(SavedState));
	*((SavedState*)app->savedState) = savedState;
	app->savedStateSize = sizeof(SavedState);
  }

  void AndroidWindow::LoadState() {
	// 以前の保存状態で開始します。復元してください。
	if (app->savedState != NULL) {
	  savedState = *static_cast<const SavedState*>(app->savedState);
	}
  }

  void AndroidWindow::GyroFunction(const Vector3F& vector, float timestamp) {
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

} // namespace Mai
