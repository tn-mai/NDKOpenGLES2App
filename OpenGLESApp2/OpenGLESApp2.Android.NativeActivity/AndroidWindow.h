#include "../../Common/Window.h"
#include "android_native_app_glue.h"

struct android_app;

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

  private:
	android_app* state;
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
  };

} // namespace Mai
