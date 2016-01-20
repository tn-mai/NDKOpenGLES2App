#ifndef MAI_COMMON_ENGINE_H_INCLUDED
#define MAI_COMMON_ENGINE_H_INCLUDED
#include "Window.h"
#include "Vector.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/SpacePartitioner.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Collision.h"
#include <boost/random.hpp>
#include <stdint.h>

//#define SHOW_DEBUG_SENSOR_OBJECT

namespace Mai {

  struct Camera {
	Camera(const Position3F& pos, const Vector3F& at, const Vector3F& up)
	  : position(pos)
	  , eyeVector(at)
	  , upVector(up)
	{}

	Position3F position;
	Vector3F eyeVector;
	Vector3F upVector;
  };

  /**
  */
  class Engine {
  public:
	enum State {
	  STATE_CONTINUE,
	  STATE_TERMINATE,
	};

	Engine(Window*);
	void InitDisplay();
	void TermDisplay();
	State Update(Window*, float);
	void DrawFrame();
	bool IsInitialized() const { return initialized; }

  private:
	void Draw();

  private:
	bool initialized;

	Window* pWindow;
	Renderer renderer;
	std::unique_ptr<SpacePartitioner> pPartitioner;
	boost::random::mt19937 random;
#ifdef SHOW_DEBUG_SENSOR_OBJECT
	ObjectPtr debugSensorObj;
#endif // SHOW_DEBUG_SENSOR_OBJECT
	ObjectPtr debugObj[3];
	Collision::RigidBodyPtr rigidCamera;

	// frame rate control.
	float avgFps;
	float deltaTime;
	int frames;
	int latestFps;
	int64_t startTime;

#ifndef NDEBUG
	bool debug;
	bool dragging;
	Camera camera;
	int mouseX;
	int mouseY;
#endif // NDEBUG
  };

} // namespace Mai

#endif // MAI_COMMON_ENGINE_H_INCLUDED
