/**
@file FailureScene.cpp
*/
#include "Scene.h"

namespace SunnySideUp {

  using namespace Mai;

  /** Show the failure event.
  */
  class FailureScene : public Scene {
  public:
	FailureScene();
	virtual ~FailureScene() {}
	virtual int Update(Engine&, float);
	virtual bool Load(Engine&);
	virtual bool Unload(Engine&);
	virtual void Draw(Engine&);
	virtual void ProcessWindowEvent(Engine&, const Event&);

  private:
	static const int eventTime = 30;
	static const int enableInputTime = eventTime - 5;

	int DoUpdate(Engine&, float);
	int DoFadeOut(Engine&, float);
	int(FailureScene::*updateFunc)(Engine&, float);

	std::vector<ObjectPtr> objList;
	bool loaded;
	bool hasFinishRequest;
	float timer;
	float cameraRotation;
  };

  FailureScene::FailureScene()
	: updateFunc(&FailureScene::DoUpdate)
	, objList()
	, loaded(false)
	, hasFinishRequest(false)
	, timer(static_cast<float>(eventTime))
	, cameraRotation(0)
  {}

  bool FailureScene::Load(Engine& engine) {
	if (loaded) {
	  status = STATUSCODE_RUNNABLE;
	  return true;
	}

	objList.reserve(8);
	Renderer& r = engine.GetRenderer();
	const Vector3F shadowDir = GetSunRayDirection(r.GetTimeOfScene());
	r.SetShadowLight(Position3F(0, 0, 0) - shadowDir * 200.0f, shadowDir, 100, 300, Vector2F(3, 3 * 4));

	{
	  auto obj = r.CreateObject("ground", Material(Color4B(255, 255, 255, 255), 0, 0), "solidmodel", ShadowCapability::Disable);
	  obj->SetRotation(degreeToRadian(-90.0f), 0, 0);
	  objList.push_back(obj);
	}
	{ // for shadow.
	  auto obj = r.CreateObject("ground", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  obj->SetTranslation(Vector3F(0, -1, 0));
	  obj->SetScale(Vector3F(0.01f, 0.01f, 0.01f));
	  obj->SetRotation(degreeToRadian(-90.0f), 0, 0);
	  objList.push_back(obj);
	}

	{
	  auto obj0 = r.CreateObject("BrokenEgg", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  Object& o0 = *obj0;
	  o0.SetScale(Vector3F(3, 3, 3));
	  o0.SetTranslation(Vector3F(0, 0.75f, 0));
	  objList.push_back(obj0);

	  auto obj2 = r.CreateObject("EggYolk", Material(Color4B(255, 255, 255, 255), -0.05f, 0), "default");
	  Object& o2 = *obj2;
	  o2.SetScale(Vector3F(3, 3, 3));
	  o2.SetTranslation(Vector3F(0, 0.75f, 0));
	  objList.push_back(obj2);

	  auto obj1 = r.CreateObject("EggWhite", Material(Color4B(255, 255, 255, 96), -0.25f, 0), "defaultWithAlpha");
	  Object& o1 = *obj1;
	  o1.SetScale(Vector3F(3, 3, 3));
	  o1.SetTranslation(Vector3F(0, 1, 0));
	  objList.push_back(obj1);
	}

	loaded = true;
	status = STATUSCODE_RUNNABLE;
	return true;
  }

  bool FailureScene::Unload(Engine&) {
	if (loaded) {
	  objList.clear();
	  loaded = false;
	}
	status = STATUSCODE_STOPPED;
	return true;
  }

  void FailureScene::ProcessWindowEvent(Engine&, const Event& e) {
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

  int FailureScene::Update(Engine& engine, float tick) {
	for (auto e : objList) {
	  e->Update(tick);
	}

	const float theta = degreeToRadian<float>(cameraRotation);
	const float distance = 20;
	const float x = std::cos(theta) * distance;
	const float z = std::sin(theta) * distance;
	const Position3F eyePos(x, 20, z);
	const Vector3F dir = objList[2]->Position() - eyePos;
	engine.GetRenderer().Update(tick, eyePos, dir, Vector3F(0, 1, 0));

	cameraRotation += tick * 10.0f;
	while (cameraRotation >= 360.0f) {
	  cameraRotation -= 360.0f;
	}

	return (this->*updateFunc)(engine, tick);
  }

  /** Wait user input.

  Transition to DoFadeOut() when receive user input or time out.
  This is the update function called from Update().
  */
  int FailureScene::DoUpdate(Engine& engine, float tick) {
	timer -= tick;
	if ((timer <= 0.0f) || hasFinishRequest) {
	  engine.GetRenderer().FadeOut(Color4B(0, 0, 0, 0), 1.0f);
	  updateFunc = &FailureScene::DoFadeOut;
	}
	return SCENEID_CONTINUE;
  }

  /** Do fade out.

  Transition to the scene of the success/failure event when the fadeout finished.
  This is the update function called from Update().
  */
  int FailureScene::DoFadeOut(Engine& engine, float deltaTime) {
	Renderer& r = engine.GetRenderer();
	if (r.GetCurrentFilterMode() == Renderer::FILTERMODE_NONE) {
	  r.FadeIn(1.0f);
	  return SCENEID_MAINGAME;
	}
	return SCENEID_CONTINUE;
  }

  void FailureScene::Draw(Engine& engine) {
	Renderer& r = engine.GetRenderer();
	const char str[] = "IT'S EDIBLE...";
	float scale = 1.0f;
	const float w = r.GetStringWidth(str) * scale;
	r.AddString(0.51f - w * 0.5f, 0.26f, scale, Color4B(20, 10, 10, 128), str);
	r.AddString(0.5f - w * 0.5f, 0.25f, scale, Color4B(200, 200, 200, 255), str);
	r.Render(&objList[0], &objList[0] + objList.size());
  }

  ScenePtr CreateFailureScene(Engine&) {
	return ScenePtr(new FailureScene());
  }

} // namespace SunnySideUp