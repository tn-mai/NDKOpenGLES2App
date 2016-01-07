#ifndef MAI_COMMON_ENGINE_H_INCLUDED
#define MAI_COMMON_ENGINE_H_INCLUDED
#include "Window.h"
#include "Vector.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Scene.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Collision.h"
#include <boost/random.hpp>
#include <stdint.h>

//#define SHOW_DEBUG_SENSOR_OBJECT

namespace Mai {

  /**
  */
  class Engine {
  public:
	Engine(Window*);
	void InitDisplay();
	void TermDisplay();
	void DrawFrame(const Position3F&, const Vector3F&, const Vector3F&);
	bool IsInitialized() const { return initialized; }

  private:
	void Draw();

  private:
	bool initialized;
	Window* pWindow;
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

} // namespace Mai

#endif // MAI_COMMON_ENGINE_H_INCLUDED
