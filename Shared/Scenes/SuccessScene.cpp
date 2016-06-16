/**
 @file SuccessScene.cpp
*/
#include "LevelInfo.h"
#include "SaveData.h"
#include "Scene.h"
#include "../../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include <vector>
#include <random>

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

	int DoUpdate(Engine&, float);
	int DoFadeOut(Engine&, float);
	int(SuccessScene::*updateFunc)(Engine&, float);

	std::vector<ObjectPtr> objList;
	bool loaded;
	bool hasFinishRequest;
	bool hasNewRecord;
	float timer;
	float cameraRotation;
  };

  SuccessScene::SuccessScene()
	: updateFunc(&SuccessScene::DoUpdate)
	, objList()
	, loaded(false)
	, hasFinishRequest(false)
	, hasNewRecord(false)
	, timer(static_cast<float>(eventTime))
	, cameraRotation(0)
  {}

  bool SuccessScene::Load(Engine& engine) {
	if (loaded) {
	  status = STATUSCODE_RUNNABLE;
	  return true;
	}

	const CommonData* pCommonData = engine.GetCommonData<CommonData>();
	hasNewRecord = SaveData::SetNewRecord(engine.GetWindow(), pCommonData->level, pCommonData->currentTime);

	objList.reserve(8);
	Renderer& r = engine.GetRenderer();
	const Vector3F shadowDir = GetSunRayDirection(r.GetTimeOfScene());
	r.SetShadowLight(Position3F(0, 0, 0) - shadowDir * 200.0f, shadowDir, 100, 300, Vector2F(3, 3 * 4));
	r.DoesDrawSkybox(false);

	{
	  static const char* const cookingNameList[] = {
		"SunnySideUp",
		"OverMedium",
	  };
	  boost::random::mt19937 random(static_cast<uint32_t>(time(nullptr)));
	  const int n = std::uniform_int_distribution<>(0, sizeof(cookingNameList)/sizeof(cookingNameList[0]) - 1)(random);
	  auto obj = r.CreateObject(cookingNameList[n], Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  obj->SetScale(Vector3F(7, 7, 7));
	  obj->SetTranslation(Vector3F(0, 2.5f, 0));
	  objList.push_back(obj);
	}
	{
	  auto obj = r.CreateObject("FlyingPan", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
	  obj->SetScale(Vector3F(10, 10, 10));
	  obj->SetTranslation(Vector3F(0, 1.25f, 0));
	  objList.push_back(obj);
	}
	{
	  auto obj = r.CreateObject("landscape", Material(Color4B(255, 255, 255, 255), 0, 0), "solidmodel" , ShadowCapability::Disable);
	  obj->SetScale(Vector3F(12, 1, 12));
	  obj->SetTranslation(Vector3F(0, -4, 0));
	  objList.push_back(obj);
	}
	{ // for shadow.
	  auto obj = r.CreateObject("ground", Material(Color4B(255, 255, 255, 255), 0, 0), "default", ShadowCapability::ShadowOnly);
	  obj->SetTranslation(Vector3F(0, -3, 0));
	  obj->SetScale(Vector3F(0.01f, 0.01f, 0.01f));
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
	default:
	  /* DO NOTING */
	  break;
	}
  }
  
  int SuccessScene::Update(Engine& engine, float tick) {
	for (auto e : objList) {
	  e->Update(tick);
	}

	const float theta = degreeToRadian<float>(cameraRotation);
	const float distance = 25;
	const float x = std::cos(theta) * distance;
	const float z = std::sin(theta) * distance;
	const Position3F eyePos(x, 20, z);
	const Vector3F dir = objList[1]->Position() - eyePos;
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
  int SuccessScene::DoUpdate(Engine& engine, float tick) {
	timer -= tick;
	if ((timer <= 0.0f) || hasFinishRequest) {
	  engine.GetRenderer().FadeOut(Color4B(0, 0, 0, 0), 1.0f);
	  updateFunc = &SuccessScene::DoFadeOut;
	}
	return SCENEID_CONTINUE;
  }
  
  /** Do fade out.

  Transition to the scene of the success/failure event when the fadeout finished.
  This is the update function called from Update().
  */
  int SuccessScene::DoFadeOut(Engine& engine, float deltaTime) {
	Renderer& r = engine.GetRenderer();
	if (r.GetCurrentFilterMode() == Renderer::FILTERMODE_NONE) {
	  r.FadeIn(1.0f);
	  CommonData& commonData = *engine.GetCommonData<CommonData>();
	  commonData.level = std::min(commonData.level + 1, GetMaximumLevel());
	  return SCENEID_MAINGAME;
	}
	return SCENEID_CONTINUE;
  }
  
  void SuccessScene::Draw(Engine& engine) {
	Renderer& r = engine.GetRenderer();
	float scale = 1.0f;
	{
	  const char str[] = "THAT'S YUMMY!";
	  const float w = r.GetStringWidth(str) * scale;
	  r.AddString(0.51f - w * 0.5f, 0.26f, scale, Color4B(20, 10, 10, 128), str);
	  r.AddString(0.5f - w * 0.5f, 0.25f, scale, Color4B(250, 250, 250, 255), str);
	}
	if (hasNewRecord) {
	  const char str[] = "NEW RECORD!";
	  const float w = r.GetStringWidth(str) * scale;
	  r.AddString(0.51f - w * 0.5f, 0.6f, scale, Color4B(20, 10, 10, 128), str);
	  r.AddString(0.5f - w * 0.5f, 0.6f, scale, Color4B(250, 100, 50, 255), str);
	}
	{
	  const char str[] = "YOUR TIME IS:";
	  const float w = r.GetStringWidth(str) * scale;
	  r.AddString(0.51f - w * 0.5f, 0.675f, scale, Color4B(20, 10, 10, 128), str);
	  r.AddString(0.5f - w * 0.5f, 0.675f, scale, Color4B(250, 250, 250, 255), str);
	}
	{
	  const float time = static_cast<float>(engine.GetCommonData<CommonData>()->currentTime) / 1000.0f;
	  char buf[32];
	  sprintf(buf, "%03.3fSec", time);
	  const float w = r.GetStringWidth(buf);
	  r.AddString(0.51f - w * 0.5f, 0.75f, scale, Color4B(20, 10, 10, 128), buf);
	  r.AddString(0.5f - w * 0.5f, 0.75f, scale, Color4B(250, 250, 250, 255), buf);
	}
	r.Render(&objList[0], &objList[0] + objList.size());
  }

  ScenePtr CreateSuccessScene(Engine&) {
	return ScenePtr(new SuccessScene());
  }

} // namespace SunnySideUp