#ifndef SCENE_SCENE_H_INCLUDED
#define SCENE_SCENE_H_INCLUDED
#include "../Engine.h"
#include <boost/math/constants/constants.hpp>

namespace SunnySideUp {

  enum UserSceneId {
	SCENEID_TITLE = Mai::SCENEID_USER,
	SCENEID_STARTEVENT,
	SCENEID_MAINGAME,
	SCENEID_SUCCESS,
	SCENEID_FAILURE,
	SCENEID_GAMEOVER,
  };

  Mai::ScenePtr CreateTitleScene(Mai::Engine&);
  Mai::ScenePtr CreateStartEventScene(Mai::Engine&);
  Mai::ScenePtr CreateMainGameScene(Mai::Engine&);
  Mai::ScenePtr CreateSuccessScene(Mai::Engine&);
  Mai::ScenePtr CreateFailureScene(Mai::Engine&);
  Mai::ScenePtr CreateGameOverScene(Mai::Engine&);

  template<typename T>
  T degreeToRadian(T d) {
	return d * boost::math::constants::pi<T>() / static_cast<T>(180);
  }

} // SunnySideUp

#endif // SCENE_SCENE_H_INCLUDED