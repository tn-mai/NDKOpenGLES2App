/**
 @file SuccessScene.cpp
*/
#include "Scene.h"
#include "../../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include <vector>

namespace SunnySideUp {

  using namespace Mai;

  /** Show the success event.
  */
  class SuccessScene : public Scene {
  public:
	SuccessScene();
	virtual ~SuccessScene() {}
	virtual int Update(Engine&, float);
	virtual bool Load(Engine&);
	virtual bool Unload(Engine&);
	virtual void Draw(Engine&);
	virtual void ProcessWindowEvent(Engine&, const Event&);

  private:
	static const int eventTime = 60;
	static const int enableInputTime = eventTime - 5;

	std::vector<ObjectPtr> objList;
	bool loaded;
	bool hasFinishRequest;
	float timer;
	float cameraRotation;
  };

  SuccessScene::SuccessScene()
	: objList()
	, loaded(false)
	, hasFinishRequest(false)
	, timer(static_cast<float>(eventTime))
	, cameraRotation(0)
  {}

  bool SuccessScene::Load(Engine& engine) {
	if (loaded) {
	  status = STATUSCODE_RUNNABLE;
	  return true;
	}

	objList.reserve(8);
	Renderer& r = engine.GetRenderer();
	{
	  auto obj = r.CreateObject("SunnySideUp", Material(Color4B(255, 255, 255, 255), 0.1f, 0.1f), "default");
	  obj->SetScale(Vector3F(7, 7, 7));
	  obj->SetTranslation(Vector3F(0, 2, 0));
	  objList.push_back(obj);
	}
	{
	  auto obj = r.CreateObject("FlyingPan", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  obj->SetScale(Vector3F(10, 10, 10));
	  obj->SetTranslation(Vector3F(0, 1, 0));
	  objList.push_back(obj);
	}
	{
	  auto obj = r.CreateObject("ground", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  obj->SetRotation(degreeToRadian(-90.0f), 0, 0);
	  objList.push_back(obj);
	}

	loaded = true;
	status = STATUSCODE_RUNNABLE;
	return true;
  }

  bool SuccessScene::Unload(Engine&) {
	if (loaded) {
	  objList.clear();
	  loaded = false;
	}
	status = STATUSCODE_STOPPED;
	return true;
  }

  void SuccessScene::ProcessWindowEvent(Engine&, const Event& e) {
	switch (e.Type) {
	case Event::EVENT_MOUSE_BUTTON_PRESSED:
	  if ((timer <= enableInputTime) && (e.MouseButton.Button == MOUSEBUTTON_LEFT)) {
		hasFinishRequest = true;
	  }
	  break;
	}
  }
  
  int SuccessScene::Update(Engine& engine, float tick) {
	for (auto e : objList) {
	  e->Update(tick);
	}

	Renderer& r = engine.GetRenderer();
	const float theta = degreeToRadian<float>(cameraRotation);
	const float distance = 25;
	const float x = std::cos(theta) * distance;
	const float z = std::sin(theta) * distance;
	const Position3F eyePos(x, 20, z);
	const Vector3F dir = objList[1]->Position() - eyePos;
	r.Update(tick, eyePos, dir, Vector3F(0, 1, 0));

	cameraRotation += tick * 10.0f;
	while (cameraRotation >= 360.0f) {
	  cameraRotation -= 360.0f;
	}

	timer -= tick;
	if ((timer <= 0.0f) || hasFinishRequest) {
	  return SCENEID_MAINGAME;
	}
	return SCENEID_CONTINUE;
  }
  
  void SuccessScene::Draw(Engine& engine) {
	Renderer& r = engine.GetRenderer();
	const char str[] = "THAT'S YUMMY!";
	float scale = 1.0f;
	const float w = r.GetStringWidth(str) * scale;
	r.AddString(0.51f - w * 0.5f, 0.26f, scale, Color4B(20, 10, 10, 128), str);
	r.AddString(0.5f - w * 0.5f, 0.25f, scale, Color4B(250, 250, 250, 255), str);
	r.Render(&objList[0], &objList[0] + objList.size());
  }

  ScenePtr CreateSuccessScene(Engine&) {
	return ScenePtr(new SuccessScene());
  }

} // namespace SunnySideUp