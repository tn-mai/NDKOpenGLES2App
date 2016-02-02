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
		  obj->SetTranslation(Vector3F(0, -5, -1));
		  //obj->SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(0), degreeToRadian<float>(0));
		  //obj->SetScale(Vector3F(5, 5, 5));
		  objList.push_back(obj);
		}
		{
		  auto obj = r.CreateObject("ChickenEgg", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		  obj->SetAnimation(r.GetAnimation("Stand"));
		  obj->SetTranslation(Vector3F(0, -10, 2));
		  obj->SetRotation(degreeToRadian<float>(-90), degreeToRadian<float>(0), degreeToRadian<float>(0));
		  objList.push_back(obj);
		}
		animeNo = 0;
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
	  r.Update(tick, Position3F(0, 0, 0), Vector3F(0, -1, 0), Vector3F(0, 0, -1));
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
	  r.Render(&objList[0], &objList[0] + objList.size());
	}
  private:
	std::vector<ObjectPtr> objList;
	int result;
	int animeNo;
	bool loaded;
  };

  ScenePtr CreateTitleScene(Engine&) {
	return ScenePtr(new TitleScene());
  }

} // namespace SunnySideUp
