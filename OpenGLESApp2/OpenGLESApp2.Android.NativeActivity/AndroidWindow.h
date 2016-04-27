#include "../../Shared/Window.h"
#include "../../Shared/Matrix.h"
#include "android_native_app_glue.h"

struct ASensorManager;
struct ASensor;
struct ASensorEventQueue;

namespace Mai {

  class AndroidWindow : public Window {
  public:
	AndroidWindow(android_app*);
	virtual ~AndroidWindow() {}
	virtual EGLNativeWindowType GetWindowType() const;
	virtual EGLNativeDisplayType GetDisplayType() const;
	virtual void MessageLoop();
	virtual bool Initialize(const char* name, size_t width, size_t height);
	virtual void Destroy();
	virtual void SetVisible(bool isVisible) {}

	virtual bool SaveUserFile(const char* filename, const void* data, size_t size);
	virtual size_t GetUserFileSize(const char* filename);
	virtual bool LoadUserFile(const char* filename, void* data, size_t size);

	void CalcFusedOrientation();
	void CommandHandler(int32_t);
	bool InputHandler(AInputEvent*);
	bool TouchSwipeHandler(AInputEvent*);
	void SaveState();
	void LoadState();
	void GyroFunction(const Vector3F& vector, int64_t timestamp);

  private:
	android_app* app;
	bool suspending;

	// for sensor.
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
	Vector3F prevOrientation;

	struct TapInfo {
	  int32_t id;
	  Vector2F pos;
	};
	struct TouchSwipeState {
	  bool dragging;
	  float dpFactor;
	  TapInfo tapInfo;
	  TapInfo dragInfo;
	} touchSwipeState;

	struct SavedState {
	} savedState;
  };

  int64_t uptimeMillis();

} // namespace Mai
