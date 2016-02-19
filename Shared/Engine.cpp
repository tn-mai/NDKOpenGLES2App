/**
 @file Engine.cpp

 Game scene flow.
 <pre>
     +---------------------------------------------------------------------------------------+
     |                                                                                       |
     |                               +--------------------------------+                      |
     |                               |  +---------------------------+ |                      |
     V                               V  V                           | |                      |
 +-------+    +-------------+    +-----------+    +---------------+ | |                      |
 | Title |--->| Start event |--->| Main game |-+->| Success event |-+ |                      |
 +-------+    +-------------+    +-----------+ |  +---------------+   |                      |
                                               |                      |                      |
                                               |  +---------------+   |  +-----------------+ |
                                               +->| Failure event |---+->| Game over event |-+
                                                  +---------------+      +-----------------+
 </pre>
*/
#include "Engine.h"
#include "Window.h"
#include <time.h>

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))
#else
#define LOGI(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#define LOGE(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#endif // __ANDROID__

namespace Mai {

  int_fast64_t get_time() {
#ifdef __ANDROID__
#if 1
	timespec tmp;
	clock_gettime(CLOCK_MONOTONIC, &tmp);
	return tmp.tv_sec * (1000UL * 1000UL * 1000UL) + tmp.tv_nsec;
#else
	static int sfd = -1;
	if (sfd == -1) {
	  sfd = open("/dev/alarm", O_RDONLY);
	}
	timespec ts;
	const int result = ioctl(sfd, ANDROID_ALARM_GET_TIME(ANDROID_ALARM_ELAPSED_REALTIME), &ts);
	if (result != 0) {
	  const int err = clock_gettime(CLOCK_MONOTONIC, &ts);
	  if (err < 0) {
		LOGI("ERROR(%d) from; clock_gettime(CLOCK_MONOTONIC)", err);
	  }
	}
	return ts.tv_sec * (1000 * 1000 * 1000) + ts.tv_nsec;
#endif
#else
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return ft.dwHighDateTime * 10000000LL + ft.dwLowDateTime * 100LL;
#endif // __ANDROID__
  }

  Engine::Engine(Window* p)
	: initialized(false)
	, pWindow(p)
	, renderer()

	, avgFps(30.0f)
	, deltaTime(1.0f / avgFps)
	, frames(0)
	, latestFps(30)
	, startTime(0)
  {
  }

  Engine::State Engine::ProcessWindowEvent(Window& window) {
	window.MessageLoop();
	while (auto e = window.PopEvent()) {
	  switch (e->Type) {
	  case Event::EVENT_CLOSED:
		TermDisplay();
		return STATE_TERMINATE;

	  case Event::EVENT_INIT_WINDOW:
		audio.reset();
		audio = CreateAudioEngine();
		InitDisplay();
		break;

	  case Event::EVENT_TERM_WINDOW:
		TermDisplay();
		audio.reset();
		break;

	  default:
		if (pCurrentScene) {
		  pCurrentScene->ProcessWindowEvent(*this, *e);
		}
		break;
	  }
	}
	return STATE_CONTINUE;
  }

  /** Register the scene creation function.

    @param id    the identify code of the scene.
	@param func  the function pointer of the scene creation.
  */
  void Engine::RegisterSceneCreator(int id, CreateSceneFunc func) {
	sceneCreatorList.insert(std::make_pair(id, func));
  }

  bool Engine::SetNextScene(int sceneId) {
	auto itr = sceneCreatorList.find(sceneId);
	if (itr != sceneCreatorList.end()) {
	  pNextScene = itr->second(*this);
	  return true;
	}
	return false;
  }

  /** Run application.

  (start)
     |
  (set to next scene)
     |
  LOADING
     |
  (when loaded, set to current scene)
     |
  RUNNABLE
     |
  (transition request. create next scene and set to next scene)
     |             |
     |          LOADING
     |             |
  (when loaded, current scene set to unloading scene, and next scene set to current scene)
     |             |
  UNLOADING     RUNNABLE
     |             |
  STOPPED          |
     |             |
   (end)           |
                   |
     +-------------+
	 |
  (suspend request)
     |
  UNLOADING
     |
  (terminate window and suspending...)
     |
  (resume request)
     |
  (initialize window)
     |
  LOADING
     |
  RUNNABLE
     |
  (terminate request)
     |
  UNLOADING
     |
  STOPPED
     |
   (end)
  */
  void Engine::Run(Window& window, int initialSceneId) {
	pCurrentScene.reset();
	if (!SetNextScene(initialSceneId)) {
	  return;
	}

	while (1) {
	  const State status = ProcessWindowEvent(window);
	  if (status == STATE_TERMINATE) {
		return;
	  }
	  if (!initialized) {
		continue;
	  }

	  // 次のシーンの準備.
	  if (!pUnloadingScene && pNextScene) {
		if (pNextScene->Load(*this)) {
		  // 準備ができたので、前のシーンをアンロード対象とし、次のシーンを現在のシーンに登録する.
		  pUnloadingScene = pCurrentScene;
		  pCurrentScene = pNextScene;
		  pNextScene.reset();
		}
	  }

	  // 前のシーンの破棄.
	  if (pUnloadingScene) {
		if (pUnloadingScene->Unload(*this)) {
		  pUnloadingScene.reset();
		}
	  }

	  // 現在のシーンを実行.
	  if (pCurrentScene) {
		switch (pCurrentScene->GetState()) {
		case Scene::STATUSCODE_LOADING:
		  pCurrentScene->Load(*this);
		  break;
		case Scene::STATUSCODE_RUNNABLE: {
#if 1
		  ++frames;
		  const int64_t curTime = get_time();
		  if (curTime < startTime) {
			startTime = curTime;
			frames = 0;
		  } else if (curTime - startTime >= (1000UL * 1000UL * 1000UL)) {
			latestFps = std::min(frames, 60);
			startTime = curTime;
			frames = 0;
			if (latestFps > avgFps * 0.3f) { // 低すぎるFPSはノイズとみなす.
			  avgFps = (avgFps + latestFps) * 0.5f;
			  deltaTime = (deltaTime + 1.0f / latestFps) * 0.5f;
			}
		  }
#endif
		  const int nextScneId = pCurrentScene->Update(*this, deltaTime);
		  switch (nextScneId) {
		  case SCENEID_TERMINATE:
			TermDisplay();
			return;
		  case SCENEID_CONTINUE:
			break;
		  default:
			SetNextScene(nextScneId);
			break;
		  }
		  break;
		}
		case Scene::STATUSCODE_UNLOADING:
		  pCurrentScene->Unload(*this);
		  break;
		case Scene::STATUSCODE_STOPPED:
		  break;
		default:
		  break;
		}
	  }
	  DrawFrame();
	}
  }

  /**
  * 現在の表示の EGL コンテキストを初期化します。
  */
  void Engine::InitDisplay() {
	// GL の状態を初期化します。
	renderer.Initialize(*pWindow);
	if (pNextScene) {
	  pNextScene->SetState(Scene::STATUSCODE_LOADING);
	}
	if (pCurrentScene) {
	  pCurrentScene->SetState(Scene::STATUSCODE_LOADING);
	}
#ifdef SHOW_DEBUG_SENSOR_OBJECT
	debugSensorObj = renderer.CreateObject("octahedron", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
#endif // SHOW_DEBUG_SENSOR_OBJECT

	LOGI("engine_init_display");
	initialized = true;
  }

  void Engine::DrawFrame() {
	if (initialized) {
	  Draw();
	}
  }
  
  /**
  * 現在ディスプレイに関連付けられている EGL コンテキストを削除します。
  */
  void Engine::TermDisplay() {
	initialized = false;
	if (pNextScene) {
	  pNextScene->Unload(*this);
	}
	if (pCurrentScene) {
	  pCurrentScene->Unload(*this);
	}
	renderer.Unload();
	LOGI("engine_term_display");
  }

  /**
  * ディスプレイ内の現在のフレームのみ。
  */
  void Engine::Draw() {
	if (!renderer.HasDisplay() || !initialized) {
	  // ディスプレイがありません。
	  return;
	}

	renderer.ClearDebugString();
	char buf[32];
#if 0
	sprintf(buf, "OX:%1.3f", fusedOrientation.x);
	renderer.AddDebugString(8, 800 - 16 * 9, buf);
	sprintf(buf, "OY:%1.3f", fusedOrientation.y);
	renderer.AddDebugString(8, 800 - 16 * 8, buf);
	sprintf(buf, "OZ:%1.3f", fusedOrientation.z);
	renderer.AddDebugString(8, 800 - 16 * 7, buf);

	sprintf(buf, "MX:%1.3f", magnet.x);
	renderer.AddDebugString(8, 800 - 16 * 6, buf);
	sprintf(buf, "MY:%1.3f", magnet.y);
	renderer.AddDebugString(8, 800 - 16 * 5, buf);
	sprintf(buf, "MZ:%1.3f", magnet.z);
	renderer.AddDebugString(8, 800 - 16 * 4, buf);

	sprintf(buf, "AX:%1.3f", accel.x);
	renderer.AddDebugString(8, 800 - 16 * 3, buf);
	sprintf(buf, "AY:%1.3f", accel.y);
	renderer.AddDebugString(8, 800 - 16 * 2, buf);
	sprintf(buf, "AZ:%1.3f", accel.z);
	renderer.AddDebugString(8, 800 - 16 * 1, buf);
#endif
	sprintf(buf, "FPS:%02d", latestFps);
	renderer.AddDebugString(8, 8, buf);
	sprintf(buf, "AVG:%02.1f", avgFps);
	renderer.AddDebugString(8, 24, buf);

	if (pCurrentScene) {
	  pCurrentScene->Draw(*this);
	}
	renderer.Swap();
  }

} // namespace Mai