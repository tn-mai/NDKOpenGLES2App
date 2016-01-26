/**
@file GameOverScene.cpp
*/
#include "Scene.h"

namespace SunnySideUp {

  using namespace Mai;

  /** Show the game over event.
*/
  class GameOverScene : public Scene {
  public:
	virtual ~GameOverScene() {}
	virtual int Update(Engine& engine, float) { return SCENEID_TITLE; }
  };

  ScenePtr CreateGameOverScene(Engine&) {
	return ScenePtr(new GameOverScene());
  }

} // namespace SunnySideUp