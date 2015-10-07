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

#include "Renderer.h"
#include "android_native_app_glue.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/sensor.h>
#include <android/log.h>
#include <android/window.h>

#include <typeinfo>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <vector>
#include <streambuf>

#include <time.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <linux/android_alarm.h>

namespace BPT = boost::property_tree;

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))

/**
* 保存状態のデータです。
*/
struct saved_state {
	float angle;
	int32_t x;
	int32_t y;
	saved_state() : angle(0), x(0), y(0) {}
};

/**
* アプリの保存状態です。
*/
struct engine {
	engine(android_app* a)
		: app(a)
		, sensorManager(0)
		, accelerometerSensor(0)
		, sensorEventQueue(0)
		, animating(0)
//		, display(0)
//		, surface(0)
//		, context(0)
//		, width(0)
//		, height(0)
		, state()
		, renderer(a)
	{}

	struct android_app* app;

	ASensorManager* sensorManager;
	const ASensor* accelerometerSensor;
	ASensorEventQueue* sensorEventQueue;

	int animating;
//	EGLDisplay display;
//	EGLSurface surface;
//	EGLContext context;
//	int32_t width;
//	int32_t height;
	struct saved_state state;

	Renderer renderer;
	Object obj[5];
};

/**
* 現在の表示の EGL コンテキストを初期化します。
*/
static int engine_init_display(struct engine* engine) {
	// OpenGL ES と EGL の初期化
#if 0
	/*4
	* 目的の構成の属性をここで指定します。
	* 以下で、オンスクリーン ウィンドウと
	* 互換性のある、各色最低 8 ビットのコンポーネントの EGLConfig を選択します
	*/
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};
	EGLint w, h, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	eglInitialize(display, 0, 0);

	/* ここで、アプリケーションは目的の構成を選択します。このサンプルでは、
	* 抽出条件と一致する最初の EGLConfig を
	* 選択する単純な選択プロセスがあります */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID は、ANativeWindow_setBuffersGeometry() に
	* よって受け取られることが保証されている EGLConfig の属性です。
	* EGLConfig を選択したらすぐに、ANativeWindow バッファーを一致させるために
	* EGL_NATIVE_VISUAL_ID を使用して安全に再構成できます。*/
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
	context = eglCreateContext(display, config, NULL, NULL);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w;
	engine->height = h;
	engine->state.angle = 0;
#endif
	// GL の状態を初期化します。
	engine->renderer.Initialize();

	engine->obj[0] = engine->renderer.CreateObject("pCube1", Material(Color4B(255, 255, 255, 255), FloatToFix8(0.04f), FloatToFix8(0.6f)), "default");
	engine->obj[0].SetAnimation(engine->renderer.GetAnimation("bend"));
	engine->obj[0].SetTranslation(Vector3F(0, 0, 0.5f));

	engine->obj[1] = engine->renderer.CreateObject("Sphere", Material(Color4B(255, 255, 255, 255), FloatToFix8(0.04f), FloatToFix8(1.0f)), "default");
	engine->obj[1].SetTranslation(Vector3F(-1.0f, 0.5f, 0.0f));

//	engine->obj[2] = engine->renderer.CreateObject("pCube1", Material(Color4B(255, 255, 255, 255), FloatToFix8(0.04f), FloatToFix8(1.0f)), "default");
//	engine->obj[2].SetAnimation(engine->renderer.GetAnimation("bend"));
//	engine->obj[2].SetTranslation(Vector3F(0, 0, 1.0f));

//	engine->obj[3] = engine->renderer.CreateObject("Sphere", Material(Color4B(255, 255, 255, 255), FloatToFix8(1.0f), FloatToFix8(0.3f)), "default");
//	engine->obj[3].SetTranslation(Vector3F(0, -1, 0));

	engine->obj[4] = engine->renderer.CreateObject("Sphere", Material(Color4B(255, 255, 255, 255), FloatToFix8(0.05f), FloatToFix8(0.1f)), "default");
	engine->obj[4].SetTranslation(Vector3F(1.0f, 0.5f, 0));

	return 0;
}

/**
* ディスプレイ内の現在のフレームのみ。
*/

static void engine_draw_frame(struct engine* engine) {
	if (!engine->renderer.HasDisplay()) {
		// ディスプレイがありません。
		return;
	}
	engine->renderer.Render(engine->obj, engine->obj + sizeof(engine->obj)/sizeof(engine->obj[0]));
	engine->renderer.Swap();
}


/**
* 現在ディスプレイに関連付けられている EGL コンテキストを削除します。
*/
static void engine_term_display(struct engine* engine) {
	engine->renderer.Unload();
}

/**
* 次の入力イベントを処理します。
*/
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* engine = (struct engine*)app->userData;
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		engine->state.x = AMotionEvent_getX(event, 0);
		engine->state.y = AMotionEvent_getY(event, 0);
		return 1;
	}
	return 0;
}

/**
* 次のメイン コマンドを処理します。
*/
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = (struct engine*)app->userData;
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
		// 現在の状態を保存するようシステムによって要求されました。保存してください。
		engine->app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)engine->app->savedState) = engine->state;
		engine->app->savedStateSize = sizeof(struct saved_state);
		break;
	case APP_CMD_INIT_WINDOW:
		// ウィンドウが表示されています。準備してください。
		if (engine->app->window != NULL) {
			engine_init_display(engine);
			engine_draw_frame(engine);
		}
		break;
	case APP_CMD_TERM_WINDOW:
		// ウィンドウが非表示または閉じています。クリーン アップしてください。
		engine_term_display(engine);
		break;
	case APP_CMD_GAINED_FOCUS:
		// アプリがフォーカスを取得すると、加速度計の監視を開始します。
		if (engine->accelerometerSensor != NULL) {
			ASensorEventQueue_enableSensor(engine->sensorEventQueue, engine->accelerometerSensor);
			// 目標は 1 秒ごとに 60 のイベントを取得することです (米国)。
			ASensorEventQueue_setEventRate(engine->sensorEventQueue, engine->accelerometerSensor, (1000L / 60) * 1000);
		}
		break;
	case APP_CMD_LOST_FOCUS:
		// アプリがフォーカスを失うと、加速度計の監視を停止します。
		// これにより、使用していないときのバッテリーを節約できます。
		if (engine->accelerometerSensor != NULL) {
			ASensorEventQueue_disableSensor(engine->sensorEventQueue, engine->accelerometerSensor);
		}
		engine_draw_frame(engine);
		break;
	}
}

int_fast64_t get_time() {
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
}

float time_to_sec(int_fast64_t time) { return static_cast<float>(static_cast<double>(time) / (1000 * 1000 * 1000));  }

/**
* これは、android_native_app_glue を使用しているネイティブ アプリケーション
* のメイン エントリ ポイントです。それ自体のスレッドでイベント ループによって実行され、
* 入力イベントを受け取ったり他の操作を実行したりします。
*/
void android_main(struct android_app* state) {
	struct engine engine(state);

	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;

	ANativeActivity_setWindowFlags(state->activity, AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

	// 加速度計の監視の準備
	engine.sensorManager = ASensorManager_getInstance();
	engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager, ASENSOR_TYPE_ACCELEROMETER);
	engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager, state->looper, LOOPER_ID_USER, NULL, NULL);

	if (state->savedState != NULL) {
		// 以前の保存状態で開始します。復元してください。
		engine.state = *(struct saved_state*)state->savedState;
	}

	engine.animating = 1;
	uint_fast64_t prevTime = get_time();

	// ループはスタッフによる開始を待っています。

	while (1) {
		// 保留中のすべてのイベントを読み取ります。
		int ident;
		int events;
		struct android_poll_source* source;

		// アニメーションしない場合、無期限にブロックしてイベントが発生するのを待ちます。
		// アニメーションする場合、すべてのイベントが読み取られるまでループしてから続行します
		// アニメーションの次のフレームを描画します。
		while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events, (void**)&source)) >= 0) {

			// このイベントを処理します。
			if (source != NULL) {
				source->process(state, source);
			}

			// センサーにデータがある場合、今すぐ処理します。
			if (ident == LOOPER_ID_USER) {
				if (engine.accelerometerSensor != NULL) {
					ASensorEvent event;
					while (ASensorEventQueue_getEvents(engine.sensorEventQueue, &event, 1) > 0) {
//						LOGI("accelerometer: x=%f y=%f z=%f",
//							event.acceleration.x, event.acceleration.y,
//							event.acceleration.z);
					}
				}
			}

			// 終了するかどうか確認します。
			if (state->destroyRequested != 0) {
				engine_term_display(&engine);
				return;
			}
		}

		if (engine.animating) {
			// イベントが完了したら次のアニメーション フレームを描画します。
#if 0
			const uint_fast64_t currentTime = get_time();
			const float deltaTime = time_to_sec(currentTime - prevTime);
			prevTime = currentTime;
#else
			const float deltaTime = 1.0f / 30.0f;
#endif
			engine.renderer.Update(deltaTime);
			for (auto&& e : engine.obj) {
				e.Update(deltaTime);
				while (e.GetCurrentTime() >= 1.0f) {
					e.SetCurrentTime(e.GetCurrentTime() - 1.0f);
				}
			}
			static float roughness = 1.0f;
			static float step = 0.005;
			engine.obj[1].SetRoughness(FloatToFix8(roughness));
#if 0
			roughness += step;
			if (roughness >= 1.0) {
				step = -0.005f;
			} else if (roughness <= 0.0) {
				step = 0.005f;
			}
#endif

			// 描画は画面の更新レートに合わせて調整されているため、
			// ここで時間調整をする必要はありません。
			engine_draw_frame(&engine);
		}
	}
}
