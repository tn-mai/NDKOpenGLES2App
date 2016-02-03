#include "Scene.h"
#include "../../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"

namespace SunnySideUp {

  using namespace Mai;

  class TitleScene : public Scene {
  public:
	TitleScene() : result(SCENEID_CONTINUE), loaded(false) {}
	virtual ~TitleScene() {}
	virtual bool Load(Engine& engine) {
	  if (!loaded) {
		objList.reserve(8);
		Renderer& r = engine.GetRenderer();
		{
		  auto obj = r.CreateObject("TitleLogo", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		  obj->SetTranslation(Vector3F(1.125f, 4.5f, -2.5f));
		  obj->SetRotation(degreeToRadian<float>(140), degreeToRadian<float>(0), degreeToRadian<float>(0));
		  //obj->SetScale(Vector3F(5, 5, 5));
		  objList.push_back(obj);
		}
		{
		  auto obj = r.CreateObject("ChickenEgg", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		  obj->SetAnimation(r.GetAnimation("Wait0"));
		  obj->SetTranslation(Vector3F(2.0f, 5, -6.1f));
		  obj->SetRotation(degreeToRadian<float>(30), degreeToRadian<float>(-30), degreeToRadian<float>(-15));
		  objList.push_back(obj);
		}
		{
		  auto obj = r.CreateObject("FlyingRock", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		  obj->SetTranslation(Vector3F(-4.5f, -5.5, -14));
		  obj->SetScale(Vector3F(3, 3, 3));
		  obj->SetRotation(degreeToRadian<float>(35), degreeToRadian<float>(-20), degreeToRadian<float>(0));
		  objList.push_back(obj);
		}
		{
		  auto obj = r.CreateObject("cloud0", Material(Color4B(255, 240, 250, 128), 0, 0), "cloud");
		  obj->SetTranslation(Vector3F(30, 0, -75));
		  obj->SetRotation(degreeToRadian<float>(90), degreeToRadian<float>(0), degreeToRadian<float>(0));
		  objList.push_back(obj);
		}
		animeNo = 0;
		scaleTick = 0;
		cloudRot = 0;
		loaded = true;
	  }
	  status = STATUSCODE_RUNNABLE;
	  return true;
	}
	virtual bool Unload(Engine&) {
	  if (loaded) {
		objList.clear();
		loaded = false;
	  }
	  status = STATUSCODE_STOPPED;
	  return true;
	}
	virtual int Update(Engine& engine, float tick) {
	  for (auto e : objList) {
		e->Update(tick);
	  }
	  Renderer& r = engine.GetRenderer();
	  r.Update(tick, Position3F(0, 0, 0), Vector3F(0.25f, 1, -1), Vector3F(0, 0, 1));
	  scaleTick += tick;
	  if (scaleTick > 2.0f) {
		scaleTick -= 2.0f;
	  }
	  cloudRot += tick;
	  if (cloudRot > 360.0f) {
		cloudRot -= 360.0f;
	  }
	  objList[3]->SetRotation(degreeToRadian<float>(90), degreeToRadian<float>(cloudRot), degreeToRadian<float>(0));
	  return result;
	}
	virtual void ProcessWindowEvent(Engine& engine, const Event& e) {
	  switch (e.Type) {
	  case Event::EVENT_MOUSE_BUTTON_PRESSED:
		if (e.MouseButton.Button == MOUSEBUTTON_LEFT) {
		  result = SCENEID_STARTEVENT;
		}
		break;
	  case Event::EVENT_KEY_PRESSED: {
		if (e.Key.Code == KEY_SPACE) {
		  static const char* const animeNameList[] = {
			"Stand", "Wait0", "Wait1", "Walk", "Dive"
		  };
		  animeNo = (animeNo + 1) % 5;
		  objList[1]->SetAnimation(engine.GetRenderer().GetAnimation(animeNameList[animeNo]));
		}
		break;
	  }
	  default:
		break;
	  }
	}
	virtual void Draw(Engine& engine) {
	  Renderer& r = engine.GetRenderer();
	  const char str[] = "TOUCH ME!";
	  float scale;
	  if (scaleTick < 1.0f) {
		scale = 1.0f + scaleTick * 0.5f;
	  } else {
		scale = 1.5f - (scaleTick - 1.0f) * 0.5f;
	  }
	  const float w = r.GetStringWidth(str) * scale;
	  r.AddString(0.5f - w * 0.5f, 0.9f, scale, Color4B(240, 240, 240, 255), str);
	  r.Render(&objList[0], &objList[0] + objList.size());
	}
  private:
	std::vector<ObjectPtr> objList;
	int result;
	int animeNo;
	bool loaded;
	float scaleTick;
	float cloudRot;
  };

  ScenePtr CreateTitleScene(Engine&) {
	return ScenePtr(new TitleScene());
  }

} // namespace SunnySideUp
