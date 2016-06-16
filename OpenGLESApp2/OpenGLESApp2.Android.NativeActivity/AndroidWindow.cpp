#include "AndroidWindow.h"
#include "../../Shared/Quaternion.h"
#include <android/window.h>
#include <android/sensor.h>
#include "android_native_app_glue.h"
#include <android/log.h>
#include <boost/math/constants/constants.hpp>
#include <boost/optional.hpp>
#include <cstdio>
#include <sys/stat.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "SunnySideUp.AndroidWindow", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "SunnySideUp.AndroidWindow", __VA_ARGS__))

/** Get an internal storage directory.

  @param app  The android native app object.

  @return An internal storage directory path name.
*/
std::string GetFilesDir(struct android_app* app) {
  JNIEnv* pJNIEnv;
  app->activity->vm->AttachCurrentThread(&pJNIEnv, nullptr);
  jclass cNativeActivity = pJNIEnv->FindClass("android/app/NativeActivity");
  jmethodID getFilesDir = pJNIEnv->GetMethodID(cNativeActivity, "getFilesDir", "()Ljava/io/File;");
  jobject file = pJNIEnv->CallObjectMethod(app->activity->clazz, getFilesDir);
  jclass cFile = pJNIEnv->FindClass("java/io/File");
  jmethodID getPath = pJNIEnv->GetMethodID(cFile, "getPath", "()Ljava/lang/String;");
  jstring path = (jstring)pJNIEnv->CallObjectMethod(file, getPath);
  const char* p = pJNIEnv->GetStringUTFChars(path, nullptr);
  app->activity->vm->DetachCurrentThread();
  return std::string(p);
}

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
	  *m = Matrix4x4::RotationZ(o.x) * Matrix4x4::RotationY(o.y) * Matrix4x4::RotationX(o.z);
	}

	void GetRotationMatrixFromQuaternion(Matrix4x4* m, const Quaternion& r) {
	  const Matrix4x3 mm = ToMatrix(r);
	  m->SetVector(0, mm.GetVector(0));
	  m->SetVector(1, mm.GetVector(1));
	  m->SetVector(2, mm.GetVector(2));
	  m->SetVector(3, Vector4F::Unit());
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

  int64_t uptimeMillis() {
	timespec tmp;
	clock_gettime(CLOCK_MONOTONIC, &tmp);
	return tmp.tv_sec * (1000UL * 1000UL * 1000UL) + tmp.tv_nsec;
  }

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
	prevOrientation = Vector3F::Unit();

	touchSwipeState.dpFactor = 160.0f / AConfiguration_getDensity(app->config);
	touchSwipeState.dragging = false;

	internalDataPath = GetFilesDir(app);
	existInternalDataPath = true;
	struct stat st;
	const int ret = stat(internalDataPath.c_str(), &st);
	if (ret < 0) {
	  if (errno == ENOENT) {
		existInternalDataPath = false;
	  }
	}
	LOGI("internalDataPath(%s): %s", existInternalDataPath ? "exist" : "not exist", internalDataPath.c_str());
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

	  const int sensorIndex = ident - LOOPER_ID_USER;
	  switch (ident) {
	  case LooperId_Accel:
		if (sensor[sensorIndex]) {
		  ASensorEvent event;
		  float timestamp = 0.0f;
		  while (ASensorEventQueue_getEvents(sensorEventQueue[sensorIndex], &event, 1) >= 0) {
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
		if (sensor[sensorIndex]) {
		  ASensorEvent event;
		  while (ASensorEventQueue_getEvents(sensorEventQueue[sensorIndex], &event, 1) >= 0) {
			magnet.x = event.magnetic.x;
			magnet.y = event.magnetic.y;
			magnet.z = event.magnetic.z;
			break;
		  }
		}
		break;
	  case LooperId_Gyro:
		if (sensor[sensorIndex]) {
		  ASensorEvent event;
		  while (ASensorEventQueue_getEvents(sensorEventQueue[sensorIndex], &event, 1) >= 0) {
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
		event.Time = uptimeMillis();
		PushEvent(event);
		return;
	  }
	}
	CalcFusedOrientation();
	if (fusedOrientation != prevOrientation) {
	  Event event;
	  event.Type = Event::EVENT_TILT;
	  event.Time = uptimeMillis();
	  event.Tilt.X = fusedOrientation.x;
	  event.Tilt.Y = fusedOrientation.y;
	  event.Tilt.Z = fusedOrientation.z;
	  PushEvent(event);
	  prevOrientation = fusedOrientation;
	  {
		Event e;
		e.Type = Event::EVENT_GYRO;
		e.Time = uptimeMillis();
		e.Gyro.X = gyro.x;
		e.Gyro.Y = gyro.y;
		e.Gyro.Z = gyro.z;
		PushEvent(e);
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

  /** Get the absolute path for the save data.

    @param filename  The save data filename.

	@return The absolute path to 'filename'.
  */
  std::string AndroidWindow::GetAbsoluteSaveDataPath(const char* filename) const {
	return internalDataPath + '/' + filename;
  }

  /** Make the directory recursively.

    @param path  The target directory path.
	@param mode  The permission(i.e. 0777).

	@retval true  Success to make 'path' directory.
	@retval false Failure to make 'path' directory.
  */
  bool MakeDirectory(const std::string& path, mode_t mode) {
	if (path.size() >= PATH_MAX - 1) {
	  LOGE("ERROR in MakeDirectory(path is too long): %s", path.c_str());
	  return false;
	}
	char tmp[PATH_MAX];
	std::copy(path.begin(), path.end(), tmp);
	tmp[path.size()] = '\0';
	if (path.back() == '/') {
	  tmp[path.size() - 1] = '\0';
	}
	for (char* p = tmp + 1; *p; ++p) {
	  if (*p == '/') {
		*p = '\0';
		mkdir(tmp, mode);
		*p = '/';
	  }
	}
	if (mkdir(tmp, mode) < 0) {
	  LOGE("ERROR in MakeDirectory(%d): %s", errno, path.c_str());
	  return false;
	}
	return true;
  }

  /** Save the user file.

	@param filename  The name of the user file.
	@param data      A pointer to the buffer where the user data is stored.
	@param size      The size of 'data' buffer.

	@retval true  Success to write the user data.
	@retval false Failure to write the user data.
  */
  bool AndroidWindow::SaveUserFile(const char* filename, const void* data, size_t size) const {
	const std::string path = GetAbsoluteSaveDataPath(filename);
	if (!existInternalDataPath) {
	  if (!MakeDirectory(internalDataPath.c_str(), 0777)) {
		LOGE("ERROR in SaveUserFile(mkdir): %s", path.c_str());
		return false;
	  }
	  LOGI("MakeDirectory: %s", internalDataPath.c_str());
	  existInternalDataPath = true;
	}
	if (FILE* fp = fopen(path.c_str(), "wb")) {
	  std::fwrite(data, size, 1, fp);
	  LOGI("Write user file(size:%d): %s", size, path.c_str());
	  fclose(fp);
	  return true;
	}
	LOGE("ERROR in SaveUserFile(size:%d): %s", size, path.c_str());
	return false;
  }

  /** Get a size of the user file.

	@param filename  The name of the user file.

	@return Byte size of the user file.
  */
  size_t AndroidWindow::GetUserFileSize(const char* filename) const {
	const std::string path = GetAbsoluteSaveDataPath(filename);
	struct stat s;
	if (stat(path.c_str(), &s) >= 0) {
	  LOGI("GetUserFileSize(%lld): %s", s.st_size, path.c_str());
	  return s.st_size;
	}
	LOGE("ERROR in GetUserFileSize: can't open %s", path.c_str());
	return 0;
  }

  /** Load the user file.

	@param filename  The name of the user file.
	@param data      A pointer to the buffer where the user data will be stored.
	@param size      The size of 'data' buffer.

	@retval true  Success to write the user data.
	@retval false Failure to write the user data.
  */
  bool AndroidWindow::LoadUserFile(const char* filename, void* data, size_t size) const {
	const std::string path = GetAbsoluteSaveDataPath(filename);
	LOGI("Read user file(size:%d): %s", size, path.c_str());
	size = std::min(size, GetUserFileSize(filename));
	if (FILE* fp = fopen(path.c_str(), "rb")) {
	  std::fread(data, size, 1, fp);
	  LOGI("Read user file(size:%d): %s", size, path.c_str());
	  fclose(fp);
	  return true;
	}
	LOGE("ERROR in LoadUserFile(size:%d): %s", size, path.c_str());
	return true;
  }

  /** Delete the user file.

	@param filename  The name of the user file.

	@retval true  Success to delete the user data.
	@retval false Failure to delete the user data.
  */
  bool AndroidWindow::DeleteUserFile(const char* filename) const {
	const std::string path = GetAbsoluteSaveDataPath(filename);
	if (unlink(path.c_str()) == 0) {
	  LOGI("Delete user file: %s", path.c_str());
	  return true;
	}
	LOGE("ERROR in DeleteUserFile(0x%x): %s", errno, path.c_str());
	return false;
  }

  void AndroidWindow::CalcFusedOrientation() {
	const float coeff = 0.02f;
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
		event.Time = uptimeMillis();
		PushEvent(event);
	  }
	  break;
	case APP_CMD_TERM_WINDOW: {
	  // ウィンドウが非表示または閉じています。クリーン アップしてください。
	  Event event;
	  event.Type = Event::EVENT_TERM_WINDOW;
	  event.Time = uptimeMillis();
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

	// only ACTION_DOWN and ACTION_UP occurs if the count is 1.
	if (count == 1) {
	  switch (action) {
	  case AMOTION_EVENT_ACTION_DOWN: {
		touchSwipeState.tapInfo.id = AMotionEvent_getPointerId(event, 0);
		touchSwipeState.tapInfo.pos = Vector2F(AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));

		Event e;
		e.Type = Event::EVENT_MOUSE_BUTTON_PRESSED;
		e.Time = AMotionEvent_getEventTime(event);
		e.MouseButton.Button = MOUSEBUTTON_LEFT;
		e.MouseButton.X = touchSwipeState.tapInfo.pos.x;
		e.MouseButton.Y = touchSwipeState.tapInfo.pos.y;
		PushEvent(e);
		consumed = true;
		break;
	  }
	  case AMOTION_EVENT_ACTION_UP: {
		const int32_t TAP_TIMEOUT = 180 * 1000000;
		const int32_t TOUCH_SLOP = 16;
		const int64_t eventTime = AMotionEvent_getEventTime(event);
		const int64_t downTime = AMotionEvent_getDownTime(event);
		if (eventTime - downTime <= TAP_TIMEOUT) {
		  if (touchSwipeState.tapInfo.id == AMotionEvent_getPointerId(event, 0)) {
			const Vector2F curPos(AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));
			if ((curPos - touchSwipeState.tapInfo.pos).Length() < TOUCH_SLOP * touchSwipeState.dpFactor) {
			  Event e;
			  e.Type = Event::EVENT_MOUSE_BUTTON_CLICKED;
			  e.Time = AMotionEvent_getEventTime(event);
			  e.MouseButton.Button = MOUSEBUTTON_LEFT;
			  e.MouseButton.X = curPos.x;
			  e.MouseButton.Y = curPos.y;
			  PushEvent(e);
			}
			Event e;
			e.Type = Event::EVENT_MOUSE_BUTTON_RELEASED;
			e.Time = AMotionEvent_getEventTime(event);
			e.MouseButton.Button = MOUSEBUTTON_LEFT;
			e.MouseButton.X = curPos.x;
			e.MouseButton.Y = curPos.y;
			PushEvent(e);
			consumed = true;
		  }
		}
		break;
	  }
	  }
	}

	{
	  switch (action) {
	  case AMOTION_EVENT_ACTION_DOWN:{
		touchSwipeState.dragInfo.id = AMotionEvent_getPointerId(event, 0);
		touchSwipeState.dragInfo.pos = Vector2F(AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));
		touchSwipeState.dragging = true;
		consumed = true;
		return true;
	  }
	  case AMOTION_EVENT_ACTION_POINTER_DOWN:
		break;
	  case AMOTION_EVENT_ACTION_UP: {
		Event e;
		e.Type = Event::EVENT_MOUSE_BUTTON_RELEASED;
		e.Time = AMotionEvent_getEventTime(event);
		e.MouseButton.Button = MOUSEBUTTON_LEFT;
		e.MouseButton.X = AMotionEvent_getX(event, 0);
		e.MouseButton.Y = AMotionEvent_getY(event, 0);
		PushEvent(e);
		touchSwipeState.dragging = false;
		consumed = true;
		return true;
	  }
	  case AMOTION_EVENT_ACTION_POINTER_UP: {
		const int id = AMotionEvent_getPointerId(event, index);
		if (touchSwipeState.dragInfo.id == id) {
		  Event e;
		  e.Type = Event::EVENT_MOUSE_BUTTON_RELEASED;
		  e.Time = AMotionEvent_getEventTime(event);
		  e.MouseButton.Button = MOUSEBUTTON_LEFT;
		  e.MouseButton.X = AMotionEvent_getX(event, index);
		  e.MouseButton.Y = AMotionEvent_getY(event, index);
		  PushEvent(e);
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
		  e.Time = AMotionEvent_getEventTime(event);
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

  void AndroidWindow::GyroFunction(const Vector3F& vector, int64_t timestamp) {
	static bool initState = true;
	static int64_t timeStamp = 0;
	if (initState) {
	  Matrix4x4 m;
	  GetRotationMatrixFromOrientation(&m, accMagOrientation);
	  gyroMatrix *= m;
	  initState = false;
	}
	Quaternion delta = Quaternion::Unit();
	if (timeStamp != 0.0f) {
	  const float NS2S = 1.0f / 1000000000.0f;
	  const float dt = static_cast<float>(timestamp - timeStamp) * NS2S;
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
