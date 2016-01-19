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
#include "../../Common/File.h"
#include "../../Common/Engine.h"

/**
* これは、android_native_app_glue を使用しているネイティブ アプリケーション
* のメイン エントリ ポイントです。それ自体のスレッドでイベント ループによって実行され、
* 入力イベントを受け取ったり他の操作を実行したりします。
*/
void android_main(android_app* app) {
  Mai::AndroidWindow window(app);
  Mai::Engine engine(&window);
  window.Initialize("Sunny Side Up", 480, 800);
  window.SetVisible(true);

  Mai::FileSystem::Initialize(app->activity->assetManager);

  while (1) {
	window.MessageLoop();
	const Mai::Engine::State state = engine.Update(&window, 1.0f / 30.0f);
	if (state == Mai::Engine::STATE_TERMINATE) {
	  break;
	}
	engine.DrawFrame();
  }
}
