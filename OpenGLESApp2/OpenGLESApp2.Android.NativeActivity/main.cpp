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

  while (1) {
	window.MessageLoop();
	const Mai::Engine::State state = engine.Update(&window, 1.0f / 30.0f);
	if (state == Mai::Engine::STATE_TERMINATE) {
	  break;
	}
	engine.DrawFrame();
  }
}
