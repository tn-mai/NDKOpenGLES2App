/*
* Copyright (C) 2010 The Android Open Source Project
*
* Apache License Version 2.0 (�u�{���C�Z���X�v) �Ɋ�Â��ă��C�Z���X����܂��B;
* �{���C�Z���X�ɏ������Ȃ��ꍇ�͂��̃t�@�C�����g�p�ł��܂���B
* �{���C�Z���X�̃R�s�[�́A�ȉ��̏ꏊ�������ł��܂��B
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* �K�p�����@�߂܂��͏��ʂł̓��ӂɂ���Ė������Ȃ�����A�{���C�Z���X�Ɋ�Â��ĔЕz�����\�t�g�E�F�A�́A
* �����َ����킸�A�����Ȃ�ۏ؂��������Ȃ��Ɍ���̂܂�
* �Еz����܂��B
* �{���C�Z���X�ł̌����Ɛ������K�肵�������ɂ��ẮA
* �{���C�Z���X���Q�Ƃ��Ă��������B
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
* �ۑ���Ԃ̃f�[�^�ł��B
*/
struct saved_state {
	float angle;
	int32_t x;
	int32_t y;
	saved_state() : angle(0), x(0), y(0) {}
};

/**
* �A�v���̕ۑ���Ԃł��B
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
* ���݂̕\���� EGL �R���e�L�X�g�����������܂��B
*/
static int engine_init_display(struct engine* engine) {
	// OpenGL ES �� EGL �̏�����
#if 0
	/*4
	* �ړI�̍\���̑����������Ŏw�肵�܂��B
	* �ȉ��ŁA�I���X�N���[�� �E�B���h�E��
	* �݊����̂���A�e�F�Œ� 8 �r�b�g�̃R���|�[�l���g�� EGLConfig ��I�����܂�
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

	/* �����ŁA�A�v���P�[�V�����͖ړI�̍\����I�����܂��B���̃T���v���ł́A
	* ���o�����ƈ�v����ŏ��� EGLConfig ��
	* �I������P���ȑI���v���Z�X������܂� */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	/* EGL_NATIVE_VISUAL_ID �́AANativeWindow_setBuffersGeometry() ��
	* ����Ď󂯎���邱�Ƃ��ۏ؂���Ă��� EGLConfig �̑����ł��B
	* EGLConfig ��I�������炷���ɁAANativeWindow �o�b�t�@�[����v�����邽�߂�
	* EGL_NATIVE_VISUAL_ID ���g�p���Ĉ��S�ɍč\���ł��܂��B*/
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
	// GL �̏�Ԃ����������܂��B
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
* �f�B�X�v���C���̌��݂̃t���[���̂݁B
*/

static void engine_draw_frame(struct engine* engine) {
	if (!engine->renderer.HasDisplay()) {
		// �f�B�X�v���C������܂���B
		return;
	}
	engine->renderer.Render(engine->obj, engine->obj + sizeof(engine->obj)/sizeof(engine->obj[0]));
	engine->renderer.Swap();
}


/**
* ���݃f�B�X�v���C�Ɋ֘A�t�����Ă��� EGL �R���e�L�X�g���폜���܂��B
*/
static void engine_term_display(struct engine* engine) {
	engine->renderer.Unload();
}

/**
* ���̓��̓C�x���g���������܂��B
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
* ���̃��C�� �R�}���h���������܂��B
*/
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = (struct engine*)app->userData;
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
		// ���݂̏�Ԃ�ۑ�����悤�V�X�e���ɂ���ėv������܂����B�ۑ����Ă��������B
		engine->app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)engine->app->savedState) = engine->state;
		engine->app->savedStateSize = sizeof(struct saved_state);
		break;
	case APP_CMD_INIT_WINDOW:
		// �E�B���h�E���\������Ă��܂��B�������Ă��������B
		if (engine->app->window != NULL) {
			engine_init_display(engine);
			engine_draw_frame(engine);
		}
		break;
	case APP_CMD_TERM_WINDOW:
		// �E�B���h�E����\���܂��͕��Ă��܂��B�N���[�� �A�b�v���Ă��������B
		engine_term_display(engine);
		break;
	case APP_CMD_GAINED_FOCUS:
		// �A�v�����t�H�[�J�X���擾����ƁA�����x�v�̊Ď����J�n���܂��B
		if (engine->accelerometerSensor != NULL) {
			ASensorEventQueue_enableSensor(engine->sensorEventQueue, engine->accelerometerSensor);
			// �ڕW�� 1 �b���Ƃ� 60 �̃C�x���g���擾���邱�Ƃł� (�č�)�B
			ASensorEventQueue_setEventRate(engine->sensorEventQueue, engine->accelerometerSensor, (1000L / 60) * 1000);
		}
		break;
	case APP_CMD_LOST_FOCUS:
		// �A�v�����t�H�[�J�X�������ƁA�����x�v�̊Ď����~���܂��B
		// ����ɂ��A�g�p���Ă��Ȃ��Ƃ��̃o�b�e���[��ߖ�ł��܂��B
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
* ����́Aandroid_native_app_glue ���g�p���Ă���l�C�e�B�u �A�v���P�[�V����
* �̃��C�� �G���g�� �|�C���g�ł��B���ꎩ�̂̃X���b�h�ŃC�x���g ���[�v�ɂ���Ď��s����A
* ���̓C�x���g���󂯎�����葼�̑�������s�����肵�܂��B
*/
void android_main(struct android_app* state) {
	struct engine engine(state);

	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;

	ANativeActivity_setWindowFlags(state->activity, AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

	// �����x�v�̊Ď��̏���
	engine.sensorManager = ASensorManager_getInstance();
	engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager, ASENSOR_TYPE_ACCELEROMETER);
	engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager, state->looper, LOOPER_ID_USER, NULL, NULL);

	if (state->savedState != NULL) {
		// �ȑO�̕ۑ���ԂŊJ�n���܂��B�������Ă��������B
		engine.state = *(struct saved_state*)state->savedState;
	}

	engine.animating = 1;
	uint_fast64_t prevTime = get_time();

	// ���[�v�̓X�^�b�t�ɂ��J�n��҂��Ă��܂��B

	while (1) {
		// �ۗ����̂��ׂẴC�x���g��ǂݎ��܂��B
		int ident;
		int events;
		struct android_poll_source* source;

		// �A�j���[�V�������Ȃ��ꍇ�A�������Ƀu���b�N���ăC�x���g����������̂�҂��܂��B
		// �A�j���[�V��������ꍇ�A���ׂẴC�x���g���ǂݎ����܂Ń��[�v���Ă��瑱�s���܂�
		// �A�j���[�V�����̎��̃t���[����`�悵�܂��B
		while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events, (void**)&source)) >= 0) {

			// ���̃C�x���g���������܂��B
			if (source != NULL) {
				source->process(state, source);
			}

			// �Z���T�[�Ƀf�[�^������ꍇ�A�������������܂��B
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

			// �I�����邩�ǂ����m�F���܂��B
			if (state->destroyRequested != 0) {
				engine_term_display(&engine);
				return;
			}
		}

		if (engine.animating) {
			// �C�x���g�����������玟�̃A�j���[�V���� �t���[����`�悵�܂��B
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

			// �`��͉�ʂ̍X�V���[�g�ɍ��킹�Ē�������Ă��邽�߁A
			// �����Ŏ��Ԓ���������K�v�͂���܂���B
			engine_draw_frame(&engine);
		}
	}
}
