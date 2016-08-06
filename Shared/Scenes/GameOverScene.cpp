/**
@file GameOverScene.cpp
*/
#include "Scene.h"
#include "Landscape.h"
#include "../Menu.h"
#include "../../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include <vector>

namespace SunnySideUp {

  using namespace Mai;

  /** Show the game over event.
*/
  class GameOverScene : public Scene {
  public:
	GameOverScene();
	virtual ~GameOverScene() {}
	virtual int Update(Engine&, float);
	virtual bool Load(Engine&);
	virtual bool Unload(Engine&);
	virtual void Draw(Engine&);
	virtual void ProcessWindowEvent(Engine&, const Event&);
  private:
	static const int eventTime = 30;
	static const int enableInputTime = eventTime - 5;

	int DoFadeIn(Engine&, float);
	int DoUpdate(Engine&, float);
	int DoFadeOut(Engine&, float);
	int(GameOverScene::*updateFunc)(Engine&, float);

	std::vector<ObjectPtr> objList;
	bool loaded;
	bool hasFinishRequest;
	float timer;
	float cameraRotation;
	Menu::Menu  rootMenu;
  };

  GameOverScene::GameOverScene()
	: updateFunc(&GameOverScene::DoFadeIn)
	, objList()
	, loaded(false)
	, hasFinishRequest(false)
	, timer(static_cast<float>(eventTime))
	, cameraRotation(180)
  {}

  bool GameOverScene::Load(Engine& engine) {
	if (loaded) {
	  status = STATUSCODE_RUNNABLE;
	  return true;
	}

	objList.reserve(8);
	Renderer& r = engine.GetRenderer();

	{
	  auto obj = r.CreateObject("EggPack", Material(Color4B(200, 200, 200, 255), 0, 0), "default");
	  obj->SetTranslation(Vector3F(-391, 16.5f, -634));
	  obj->SetRotation(degreeToRadian(0.0f), degreeToRadian(10.0f), degreeToRadian(5.0f));
	  objList.push_back(obj);
	}

	const Landscape::ObjectList landscapeObjList = Landscape::GetCoast(r, Vector3F(0, 0, 0), 12);
	objList.insert(objList.end(), landscapeObjList.begin(), landscapeObjList.end());
	{ // for shadow.
	  auto obj = r.CreateObject("ground", Material(Color4B(255, 255, 255, 255), 0, 0), "default", ShadowCapability::ShadowOnly);
	  obj->SetTranslation(Vector3F(objList[0]->Position() + Vector3F(0, -10, 0)));
	  obj->SetScale(Vector3F(0.01f, 0.01f, 0.01f));
	  obj->SetRotation(degreeToRadian(-90.0f), 0, 0);
	  objList.push_back(obj);
	}

	rootMenu.Add(Menu::MenuItem::Pointer(new Menu::TextMenuItem("GAME OVER", Vector2F(0.5f, 0.25f), 1.2f, Color4B(250, 250, 250, 255))));

	const Vector3F shadowDir = GetSunRayDirection(r.GetTimeOfScene());
	r.SetShadowLight(objList[0]->Position() - shadowDir * 200.0f, shadowDir, 160, 240, Vector2F(4, 4 * 4));
	r.DoesDrawSkybox(false);

	loaded = true;
	status = STATUSCODE_RUNNABLE;
	return true;
  }

  bool GameOverScene::Unload(Engine&) {
	if (loaded) {
	  objList.clear();
	  loaded = false;
	}
	rootMenu.Clear();
	status = STATUSCODE_STOPPED;
	return true;
  }

  void GameOverScene::ProcessWindowEvent(Engine&, const Event& e) {
	switch (e.Type) {
	case Event::EVENT_MOUSE_BUTTON_PRESSED:
	  if ((timer <= enableInputTime) && (e.MouseButton.Button == MOUSEBUTTON_LEFT)) {
		hasFinishRequest = true;
	  }
	  break;
	default:
	  /* DO NOTING */
	  break;
	}
  }

  int GameOverScene::Update(Engine& engine, float tick) {
	for (auto e : objList) {
	  e->Update(tick);
	}

	rootMenu.Update(engine, tick);

	const float theta = degreeToRadian<float>(cameraRotation);
	static const float distance = 25;
	static float height = 10;
	const float x = std::cos(theta) * distance;
	const float z = std::sin(theta) * distance;
	const Position3F eyePos(objList[0]->Position() + Vector3F(x, height, z));
	const Vector3F dir = objList[0]->Position() - eyePos;
	engine.GetRenderer().Update(tick, eyePos, dir, Vector3F(0, 1, 0));

	height += tick * 1.0f;
	cameraRotation += tick * 10.0f;
	while (cameraRotation >= 360.0f) {
	  cameraRotation -= 360.0f;
	}

	return (this->*updateFunc)(engine, tick);
  }

  /** Wait fade in.

  Transition to DoUpdate() when the fadein finished.
  This is the update function called from Update().
  */
  int GameOverScene::DoFadeIn(Engine& engine, float tick) {
	timer -= tick;
	if (engine.GetRenderer().GetCurrentFilterMode() == Renderer::FILTERMODE_NONE) {
	  engine.GetAudio().PlayBGM("Audio/gameover.mp3", 1.0f);
	  engine.GetAudio().SetBGMLoopFlag(false);
	  updateFunc = &GameOverScene::DoUpdate;
	}
	return SCENEID_CONTINUE;
  }

  /** Wait user input.

  Transition to DoFadeOut() when receive user input or time out.
  This is the update function called from Update().
  */
  int GameOverScene::DoUpdate(Engine& engine, float tick) {
	timer -= tick;
	if ((timer <= 0.0f) || hasFinishRequest) {
	  engine.GetAudio().PlaySE(Menu::SEID_Confirm, 1.0f);
	  engine.GetRenderer().FadeOut(Color4B(0, 0, 0, 0), 1.0f);
	  updateFunc = &GameOverScene::DoFadeOut;
	}
	return SCENEID_CONTINUE;
  }

  /** Do fade out.

  Transition to the scene of the success/failure event when the fadeout finished.
  This is the update function called from Update().
  */
  int GameOverScene::DoFadeOut(Engine& engine, float deltaTime) {
	Renderer& r = engine.GetRenderer();
	if (r.GetCurrentFilterMode() == Renderer::FILTERMODE_NONE) {
	  r.FadeIn(1.0f);
	  return SCENEID_TITLE;
	}
	return SCENEID_CONTINUE;
  }

  void GameOverScene::Draw(Engine& engine) {
	Renderer& r = engine.GetRenderer();
	rootMenu.Draw(r, Vector2F(0, 0), 1.0f);
	r.Render(&objList[0], &objList[0] + objList.size());
  }

  ScenePtr CreateGameOverScene(Engine&) {
	return ScenePtr(new GameOverScene());
  }

} // namespace SunnySideUp