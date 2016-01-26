/**
@file FailureScene.cpp
*/
#include "Scene.h"

namespace SunnySideUp {

  using namespace Mai;

  /** Show the failure event.
  */
  class FailureScene : public Scene {
  public:
	virtual ~FailureScene() {}
	virtual int Update(Engine& engine, float) { return SCENEID_MAINGAME; }
  };

  ScenePtr CreateFailureScene(Engine&) {
	return ScenePtr(new FailureScene());
  }

} // namespace SunnySideUp