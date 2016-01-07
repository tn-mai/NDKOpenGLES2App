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
#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))

struct Camera {
  Camera(const Mai::Position3F& pos, const Mai::Vector3F& at, const Mai::Vector3F& up)
	: position(pos)
	, eyeVector(at)
	, upVector(up)
  {}

  Mai::Position3F position;
  Mai::Vector3F eyeVector;
  Mai::Vector3F upVector;
};

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

  Camera camera(Mai::Position3F(0, 0, 0), Mai::Vector3F(0, 0, -1), Mai::Vector3F(0, 1, 0));
  int mouseX = -1, mouseY = -1;
  while (1) {
	window.MessageLoop();
	window.CalcFusedOrientation();
	while (auto e = window.PopEvent()) {
	  switch (e->Type) {
	  case Event::EVENT_CLOSED:
		engine.TermDisplay();
		return;
	  case Event::EVENT_INIT_WINDOW:
		engine.InitDisplay();
		break;
	  case Event::EVENT_TERM_WINDOW:
		engine.TermDisplay();
		break;
	  case Event::EVENT_MOUSE_MOVED:
		if (mouseX >= 0) {
		  const float x = static_cast<float>(mouseX - e->MouseMove.X) * 0.005f;
		  const float y = static_cast<float>(mouseY - e->MouseMove.Y) * 0.005f;
		  const Mai::Vector3F leftVector = camera.eyeVector.Cross(camera.upVector).Normalize();
		  camera.eyeVector = (Mai::Quaternion(camera.upVector, x) * Mai::Quaternion(leftVector, y)).Apply(camera.eyeVector).Normalize();
		  camera.upVector = Mai::Quaternion(leftVector, y).Apply(camera.upVector).Normalize();
		}
		mouseX = e->MouseMove.X;
		mouseY = e->MouseMove.Y;
		break;
	  default:
		break;
	  }
	}
	engine.DrawFrame(camera.position, camera.eyeVector, camera.upVector);
  }
}
