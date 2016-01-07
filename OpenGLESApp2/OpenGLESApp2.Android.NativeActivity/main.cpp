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
* ����́Aandroid_native_app_glue ���g�p���Ă���l�C�e�B�u �A�v���P�[�V����
* �̃��C�� �G���g�� �|�C���g�ł��B���ꎩ�̂̃X���b�h�ŃC�x���g ���[�v�ɂ���Ď��s����A
* ���̓C�x���g���󂯎�����葼�̑�������s�����肵�܂��B
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
