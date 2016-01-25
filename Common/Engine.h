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

  private:
	State ProcessWindowEvent(Window&);
	void Draw();
	bool SetNextScene(int);

  private:
	bool initialized;

	Window* pWindow;
	Renderer renderer;

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
	virtual bool Load(Engine&) { 
	  status = STATUSCODE_RUNNABLE;
	  return true;
	}
	virtual bool Unload(Engine&) {
	  status = STATUSCODE_STOPPED;
	  return true;
	}
	virtual void Draw(Engine&) {};
	virtual void ProcessWindowEvent(Engine&, const Event&) {};

	/** Update scene.
	  this is the pure virtual, so must implement.
	  the implementation should return following code, otherwise the next scene id.
	  - SCENEID_CONTINUE           the current scene is continued.
	  - SCENEID_TERMINATE          the application is shuting down by user request.
	  
	  the scene id has higher equal SCENEID_USER. you return the scene id if you want to transite the other scene.
	  as a result, the current scene will be terminated.
	*/
	virtual int Update(Engine&, float) = 0;

	StatusCode GetState() const { return status; }
	void SetState(StatusCode n) { status = n; }

	StatusCode status;
  };

  /** The scene identify code.
  */
  enum SceneId {
	SCENEID_USER = 0, ///< the initial code for user definition scene id.
	SCENEID_CONTINUE = -1, ///< the scene status code when current scene is continued.
	SCENEID_TERMINATE = -2, ///< the scene status code when the application is terminated.
  };

  enum UserSceneId {
	SCENEID_TITLE = SCENEID_USER,
	SCENEID_STARTEVENT,
	SCENEID_MAINGAME,
	SCENEID_SUCCESS,
	SCENEID_FAILURE,
	SCENEID_GAMEOVER,
  };

  ScenePtr CreateTitleScene(Engine&);
  ScenePtr CreateStartEventScene(Engine&);
  ScenePtr CreateMainGameScene(Engine&);
  ScenePtr CreateSuccessScene(Engine&);
  ScenePtr CreateFailureScene(Engine&);
  ScenePtr CreateGameOverScene(Engine&);

} // namespace Mai

#endif // MAI_COMMON_ENGINE_H_INCLUDED
