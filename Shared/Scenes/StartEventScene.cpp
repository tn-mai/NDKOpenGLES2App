#include "Scene.h"

namespace SunnySideUp {

  class StartEventScene : public Mai::Scene {
  public:
	virtual ~StartEventScene() {}
	virtual int Update(Mai::Engine& engine, float) { return SCENEID_MAINGAME; }
  };

  Mai::ScenePtr CreateStartEventScene(Mai::Engine&) {
	return Mai::ScenePtr(new StartEventScene);
  }

} // namespace SunnySideUp