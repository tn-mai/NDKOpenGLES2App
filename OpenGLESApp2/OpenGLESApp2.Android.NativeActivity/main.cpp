/*
* Copyright (C) 2010 The Android Open Source Project
*
* Apache License Version 2.0 (「本ライセンス」) に基づいてライセンスされます。;
* 本ライセンスに準拠しない場合はこのファイルを使用できません。
* 本ライセンスのコピーは、以下の場所から入手できます。
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* 適用される法令または書面での同意によって命じられない限り、本ライセンスに基づいて頒布されるソフトウェアは、
* 明示黙示を問わず、いかなる保証も条件もなしに現状のまま
* 頒布されます。
* 本ライセンスでの権利と制限を規定した文言については、
* 本ライセンスを参照してください。
*
*/

#include "AndroidWindow.h"
#include "../../Shared/File.h"
#include "../../Shared/Engine.h"
#include "../../Shared/Scenes/Scene.h"

/**
* これは、android_native_app_glue を使用しているネイティブ アプリケーション
* のメイン エントリ ポイントです。それ自体のスレッドでイベント ループによって実行され、
* 入力イベントを受け取ったり他の操作を実行したりします。
*/
void android_main(android_app* app) {

  namespace SSU = SunnySideUp;

  Mai::AndroidWindow window(app);
  Mai::Engine engine(&window);
  window.Initialize("Sunny Side Up", 480, 800);
  window.SetVisible(true);

  Mai::FileSystem::Initialize(app->activity->assetManager);

  engine.RegisterSceneCreator(SSU::SCENEID_TITLE, SSU::CreateTitleScene);
  engine.RegisterSceneCreator(SSU::SCENEID_STARTEVENT, SSU::CreateStartEventScene);
  engine.RegisterSceneCreator(SSU::SCENEID_MAINGAME, SSU::CreateMainGameScene);
  engine.RegisterSceneCreator(SSU::SCENEID_SUCCESS, SSU::CreateSuccessScene);
  engine.RegisterSceneCreator(SSU::SCENEID_FAILURE, SSU::CreateFailureScene);
  engine.RegisterSceneCreator(SSU::SCENEID_GAMEOVER, SSU::CreateGameOverScene);
  engine.Run(window, SSU::SCENEID_TITLE);
}
