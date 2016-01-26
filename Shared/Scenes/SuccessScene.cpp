/**
 @file SuccessScene.cpp
*/
#include "Scene.h"

namespace SunnySideUp {

  using namespace Mai;

  /** Show the success event.
  */
  class SuccessScene : public Scene {
  public:
	virtual ~SuccessScene() {}
	virtual int Update(Engine& engine, float) { return SCENEID_MAINGAME; }
  };

  ScenePtr CreateSuccessScene(Engine&) {
	return ScenePtr(new SuccessScene());
  }

} // namespace SunnySideUp