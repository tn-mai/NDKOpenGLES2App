#ifndef MAI_COMMON_ENGINE_H_INCLUDED
#define MAI_COMMON_ENGINE_H_INCLUDED
#include "Window.h"
#include "Vector.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/SpacePartitioner.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Collision.h"
#include <boost/random.hpp>
#include <memory>
#include <map>
#include <stdint.h>

//#define SHOW_DEBUG_SENSOR_OBJECT

namespace Mai {

  class Scene;
  typedef std::shared_ptr<Scene> ScenePtr;

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
	typedef ScenePtr(*CreateSceneFunc)(Engine&);

	Engine(Window*);
	void RegisterSceneCreator(int, CreateSceneFunc);
	void Run(Window&, int);
	void InitDisplay();
	void TermDisplay();
	State Update(Window*, float);
	void DrawFrame();
	bool IsInitialized() const { return renderer.HasDisplay() && initialized; }

	Renderer& GetRenderer() { return renderer; }
	const Renderer& GetRenderer() const { return renderer; }
	void InsertObject(const ObjectPtr& obj, const Collision::RigidBodyPtr& c = Collision::RigidBodyPtr(), const Vector3F& offset = Vector3F::Unit()) {
	  if (pPartitioner) {
		pPartitioner->Insert(obj, c, offset);
	  }
	}

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

	// scene control.
	std::map<int, CreateSceneFunc> sceneCreatorList;
	ScenePtr pCurrentScene;
	ScenePtr pNextScene;
	ScenePtr pUnloadingScene;

#ifndef NDEBUG
	bool debug;
	bool dragging;
	Camera camera;
	int mouseX;
	int mouseY;
#endif // NDEBUG
  };

  /** A part of the game flow.

  The scene controls updating, rendering, user input, and scene flow.
  It also has some resources like the rendering resources, sound resources, etc.
  */
  class Scene {
  public:
	/** The scene has a status.
	*/
	enum StatusCode {
	  STATUSCODE_LOADING, ///< the scene is constructed. and it is in preparation.
	  STATUSCODE_RUNNABLE, ///< the scene is prepared. it can run.
	  STATUSCODE_UNLOADING, ///< the scene is finalizing.
	  STATUSCODE_STOPPED, ///< the scene is finalized.it shuoud be removed as soon as possible.
	};

	Scene() : status(STATUSCODE_LOADING) {}
	virtual ~Scene() {}
	virtual bool Load(Engine&) = 0;
	virtual bool Unload(Engine&) = 0;
	virtual int Update(Engine&, Window&, float) = 0;
	virtual void Draw(Engine&) = 0;
	StatusCode GetState() const { return status; }

	StatusCode status;
  };

  /** The scene identify code.
  */
  enum SceneId {
	SCENEID_USER = 0, ///< the initial code for user definition scene id.
	SCENEID_CONTINUE = -1, ///< the scene status code when current scene is continued.
	SCENEID_TERMINATE = -2, ///< the scene status code when the application is terminated.
  };

} // namespace Mai

#endif // MAI_COMMON_ENGINE_H_INCLUDED
