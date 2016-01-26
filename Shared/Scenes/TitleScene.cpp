#include "Scene.h"
#include "../../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"

namespace SunnySideUp {

  class TitleScene : public Mai::Scene {
  public:
	TitleScene() : result(Mai::SCENEID_CONTINUE), loaded(false) {}
	virtual ~TitleScene() {}
	virtual bool Load(Mai::Engine& engine) {
	  if (!loaded) {
		objList.reserve(8);
		Mai::Renderer& r = engine.GetRenderer();
		auto obj = r.CreateObject("TitleLogo", Mai::Material(Mai::Color4B(255, 255, 255, 255), 0, 0), "default");
		obj->SetTranslation(Mai::Vector3F(0, -5, 0));
		//obj->SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(0), degreeToRadian<float>(0));
		//obj->SetScale(Vector3F(5, 5, 5));
		objList.push_back(obj);
		loaded = true;
	  }
	  status = STATUSCODE_RUNNABLE;
	  return true;
	}
	virtual bool Unload(Mai::Engine&) {
	  if (loaded) {
		objList.clear();
		loaded = false;
	  }
	  status = STATUSCODE_STOPPED;
	  return true;
	}
	virtual int Update(Mai::Engine& engine, float tick) {
	  Mai::Renderer& r = engine.GetRenderer();
	  r.Update(tick, Mai::Position3F(0, 0, 0), Mai::Vector3F(0, -1, 0), Mai::Vector3F(0, 0, -1));
	  return result;
	}
	virtual void ProcessWindowEvent(Mai::Engine& engine, const Mai::Event& e) {
	  switch (e.Type) {
	  case Mai::Event::EVENT_MOUSE_BUTTON_PRESSED:
		if (e.MouseButton.Button == MOUSEBUTTON_LEFT) {
		  result = SCENEID_STARTEVENT;
		}
		break;
	  default:
		break;
	  }
	}
	virtual void Draw(Mai::Engine& engine) {
	  Mai::Renderer& r = engine.GetRenderer();
	  r.Render(&objList[0], &objList[0] + objList.size());
	}
  private:
	std::vector<Mai::ObjectPtr> objList;
	int result;
	bool loaded;
  };

  Mai::ScenePtr CreateTitleScene(Mai::Engine&) {
	return Mai::ScenePtr(new TitleScene());
  }

} // namespace SunnySideUp
