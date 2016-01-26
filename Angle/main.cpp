#include "Win32Window.h"
#include "../Shared/File.h"
#include "../Shared/Engine.h"

int main() {
  Mai::Win32Window  window;
  Mai::Engine engine(&window);
  window.Initialize("Sunny Side Up", 480, 800);
  window.SetVisible(true);

  Mai::FileSystem::Initialize();

  engine.RegisterSceneCreator(Mai::SCENEID_TITLE, Mai::CreateTitleScene);
  engine.RegisterSceneCreator(Mai::SCENEID_STARTEVENT, Mai::CreateStartEventScene);
  engine.RegisterSceneCreator(Mai::SCENEID_MAINGAME, Mai::CreateMainGameScene);
  engine.RegisterSceneCreator(Mai::SCENEID_SUCCESS, Mai::CreateSuccessScene);
  engine.RegisterSceneCreator(Mai::SCENEID_FAILURE, Mai::CreateFailureScene);
  engine.RegisterSceneCreator(Mai::SCENEID_GAMEOVER, Mai::CreateGameOverScene);
  engine.Run(window, Mai::SCENEID_TITLE);

  return 0;
}