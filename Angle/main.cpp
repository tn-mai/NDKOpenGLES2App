#include "Win32Window.h"
#include "../Shared/File.h"
#include "../Shared/Engine.h"
#include "../Shared/Scenes/Scene.h"

int main() {

  namespace SSU = SunnySideUp;

  Mai::Win32Window  window;
  Mai::Engine engine(&window);
  window.Initialize("Sunny Side Up", 480, 800);
  window.SetVisible(true);

  Mai::FileSystem::Initialize();

  engine.RegisterSceneCreator(SSU::SCENEID_TITLE, SSU::CreateTitleScene);
  engine.RegisterSceneCreator(SSU::SCENEID_STARTEVENT, SSU::CreateStartEventScene);
  engine.RegisterSceneCreator(SSU::SCENEID_MAINGAME, SSU::CreateMainGameScene);
  engine.RegisterSceneCreator(SSU::SCENEID_SUCCESS, SSU::CreateSuccessScene);
  engine.RegisterSceneCreator(SSU::SCENEID_FAILURE, SSU::CreateFailureScene);
  engine.RegisterSceneCreator(SSU::SCENEID_GAMEOVER, SSU::CreateGameOverScene);
  engine.CreateCommonData<SSU::CommonData>();
  engine.Run(window, SSU::SCENEID_TITLE);

  return 0;
}