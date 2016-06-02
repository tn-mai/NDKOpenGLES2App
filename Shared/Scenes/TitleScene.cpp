#include "Scene.h"
#include "SaveData.h"
#include "../../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include <vector>
#include <memory>

#ifndef NDEBUG
//#define SSU_DEBUG_DISPLAY_GYRO
#endif // NDEBUG

namespace SunnySideUp {

  using namespace Mai;

  /** The base class of menu items.

    All menu item classes should inherit this class.
  */
  struct MenuItem {
	typedef std::shared_ptr<MenuItem> Pointer;

	virtual ~MenuItem() = 0 {}
	virtual void Draw(Renderer&, Vector2F) const = 0;
	virtual void Update(float) = 0;
  };

  /** A text menu item.
  */
  struct TextMenuItem : public MenuItem {
	TextMenuItem(const char* str, const Vector2F& p, float s) : pos(p), transparency(1.0f), baseScale(s), scaleTick(0) {
	  strncpy(label, str, 15);
	  label[15] = '\0';
	}
	virtual ~TextMenuItem() {}
	virtual void Draw(Renderer& r, Vector2F offset) const {
	  offset += pos;
	  float scale;
	  if (scaleTick < 1.0f) {
		scale = 1.0f + scaleTick * 0.5f;
	  } else {
		scale = 1.5f - (scaleTick - 1.0f) * 0.5f;
	  }
	  scale *= baseScale;
	  const float w = r.GetStringWidth(label) * scale * 0.5f;
	  r.AddString(offset.x - w, offset.y, scale, Color4B(240, 240, 240, static_cast<uint8_t>(255.0f * transparency)), label);
	}
	virtual void Update(float tick) {
	  scaleTick += tick;
	  if (scaleTick > 2.0f) {
		scaleTick -= 2.0f;
	  }
	}

	Vector2F pos;
	float transparency;
	float baseScale;
	float scaleTick;
	char label[16];
  };

  /** Menu container.

    Usually, this is used as the root of menu.
  */
  struct Menu : public MenuItem {
	Menu() : pos(0, 0) {}
	virtual ~Menu() {}
	virtual void Draw(Renderer& r, Vector2F offset) const {
	  offset += pos;
	  for (auto& e : items) {
		e->Draw(r, offset);
	  }
	}
	virtual void Update(float tick) {
	  for (auto& e : items) {
		e->Update(tick);
	  }
	}

	void Add(Menu::Pointer& p) { items.push_back(p); }
	void Clear() { items.clear(); }

	Vector2F pos;
	std::vector<MenuItem::Pointer> items;
  };

  /** Multiple menu item container.
  */
  struct CarouselMenu : public MenuItem {
	CarouselMenu() : pos(0, 0), windowSize(0), topOfWindow(0) {}
	virtual ~CarouselMenu() {}
	virtual void Draw(Renderer& r, Vector2F offset) const {
	  offset += pos;
	  const size_t containerSize = items.size();
	  for (size_t i = 0; i < windowSize; ++i) {
		auto& e = items[i % containerSize];
		e->Draw(r, offset + Vector2F(0, static_cast<float>(i) * 0.1f));
	  }
	}
	virtual void Update(float tick) {
	  for (auto& e : items) {
		e->Update(tick);
	  }
	}

	void Add(Menu::Pointer& p) { items.push_back(p); }

	Vector2F pos;
	std::vector<MenuItem::Pointer> items;
	size_t windowSize;
	size_t topOfWindow;
  };

  /** Title scene.

	+----------+
	|          | - Touch: Transition to the level selection menu.
	| SUNNY    |
	|  SIDE UP |
	|          |
	| TOUCH ME |
	|          |
	+----------+

	+----------+
	|   ...    | - Swipe: Select playing level.
	|  level1  | - Touch: Determine playing level and transition to the main game scene.
	| LEVEL2   | - Touch on REC icon: Transition to the record viewr.
	|  level3  |
	|   ...    | NOTE: Each level includes some courses.
	|REC)      |
	+----------+

	+----------+
	|. .. .. ..| - Swipe: Rotate record list.
	|2 00:00:00| - Touch on RET icon: Transition to the level selection menu.
	|3 00:00:00| - Touch on DEL icon: Transition to deleting record menu.
	|4 00:00:00|
	|. .. .. ..|
	|DEL)  (RET|
	+----------+

	+----------+
	|DELETE ALL| - Touch on YES icon: Delete all recodes and transition to the level selection menu.
	| RECORDS? | - Touch on NO icon: transition to the record viewr.
	|          |
	|   YES    |
	|   NO     |
	|          |
	+----------+

  */
  class TitleScene : public Scene {
  public:
	TitleScene()
	  : loaded(false)
	  , updateFunc(&TitleScene::DoFadeIn)
	{}

	virtual ~TitleScene() {}

	virtual bool Load(Engine& engine) {
	  if (!loaded) {
		SaveData::PrepareSaveData(engine.GetWindow());

		eyePos = Position3F(8, 19.5f, 5);
		eyeDir = Vector3F(-0.6f, 0.06f, -0.84f).Normalize();
		objList.reserve(8);
		Renderer& r = engine.GetRenderer();

		static const TimeOfScene sceneIncidence[] = {
		  TimeOfScene_Noon, TimeOfScene_Noon,
		  TimeOfScene_Sunset, TimeOfScene_Sunset, TimeOfScene_Sunset,
		  TimeOfScene_Night,
		};
		boost::random::mt19937 random(static_cast<uint32_t>(time(nullptr)));
		r.SetTimeOfScene(sceneIncidence[random() % (sizeof(sceneIncidence)/sizeof(sceneIncidence[0]))]);
		r.DoesDrawSkybox(true);

		{
		  auto obj = r.CreateObject("TitleLogo", Material(Color4B(255, 255, 255, 255), 0, 0), "default", ShadowCapability::Disable);
		  obj->SetTranslation(Vector3F(5.1f, 21.2f, 1.0f));
		  obj->SetRotation(degreeToRadian<float>(13), degreeToRadian<float>(33), degreeToRadian<float>(2));
		  //obj->SetScale(Vector3F(5, 5, 5));
		  objList.push_back(obj);
		}
		{
		  auto obj = r.CreateObject("ChickenEgg", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		  obj->SetAnimation(r.GetAnimation("Wait0"));
		  obj->SetTranslation(Vector3F(3.9f, 19.25f, -2));
		  obj->SetRotation(degreeToRadian<float>(-6), degreeToRadian<float>(11), degreeToRadian<float>(1));
		  objList.push_back(obj);
		}
		{
		  auto obj = r.CreateObject("FlyingRock", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		  obj->SetTranslation(Vector3F(4, 11.25, -3.5));
		  obj->SetScale(Vector3F(2, 2, 2));
		  obj->SetRotation(degreeToRadian<float>(5), degreeToRadian<float>(22+180), degreeToRadian<float>(-14));
		  objList.push_back(obj);
		}
		{
		  auto obj = r.CreateObject("rock_s", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		  obj->SetTranslation(Vector3F(4, 19, -5));
		  objList.push_back(obj);
		}
		{
		  auto obj = r.CreateObject("rock_s", Material(Color4B(255, 255, 255, 224), 0, 0), "default");
		  obj->SetTranslation(Vector3F(-15, 30, -20));
		  objList.push_back(obj);
		}
		{
		  auto obj = r.CreateObject("cloud0", Material(Color4B(255, 240, 250, 128), 0, 0), "cloud", ShadowCapability::Disable);
		  obj->SetTranslation(Vector3F(-30, 0, -75));
		  //obj->SetRotation(degreeToRadian<float>(90), degreeToRadian<float>(0), degreeToRadian<float>(0));
		  objList.push_back(obj);
		}

#ifdef SSU_DEBUG_DISPLAY_GYRO
		vecGyro = Vector3F::Unit();
		objGyro = r.CreateObject("unitbox", Material(Color4B(255, 255, 255, 224), 0, 0), "default");
		objGyro->SetScale(Vector3F(1, 0.25f, 0.5f));
		objList.push_back(objGyro);
#endif // SSU_DEBUG_DISPLAY_GYRO

		const Vector3F shadowDir = GetSunRayDirection(r.GetTimeOfScene());
		r.SetShadowLight(objList[0]->Position() - shadowDir * 200.0f, shadowDir, 100, 300, Vector2F(8, 8 * 4));

		pTouchMeItem.reset(new TextMenuItem("TOUCH ME!", Vector2F(0.5f, 0.7f), 1.0f));

		animeNo = 0;
		cloudRot = 0;
		loaded = true;
	  }
	  status = STATUSCODE_RUNNABLE;
	  return true;
	}

	virtual bool Unload(Engine&) {
	  if (loaded) {
#ifdef SSU_DEBUG_DISPLAY_GYRO
		objGyro.reset();
#endif // SSU_DEBUG_DISPLAY_GYRO
		objList.clear();
		pTouchMeItem.reset();
		levelMenu.Clear();
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
	  r.Update(tick, eyePos, eyeDir, Vector3F(0, 1, 0));

	  return (this->*updateFunc)(engine, tick);
	}

	/** Do fade in.

	Transition to DoUpdate() when the fadein finished.
	This is the update function called from Update().
	*/
	int DoFadeIn(Engine& engine, float) {
	  if (engine.IsInitialized()) {
		Renderer& r = engine.GetRenderer();
		r.SetFilterColor(Color4B(0, 0, 0, 255));
		r.FadeIn(1.0f);
		AudioInterface& audio = engine.GetAudio();
		audio.PlayBGM("Audio/title.mp3", 1.0f);
		updateFunc = &TitleScene::DoUpdate;
	  }
	  return SCENEID_CONTINUE;
	}

	/** Wait user input.

	Transition to DoFadeOut() when receive user input.
	This is the update function called from Update().
	*/
	int DoUpdate(Engine& engine, float tick) {
	  if (pTouchMeItem) {
		pTouchMeItem->Update(tick);
	  }

	  cloudRot += tick;
	  if (cloudRot > 360.0f) {
		cloudRot -= 360.0f;
	  }
	  objList[5]->SetRotation(degreeToRadian<float>(90), degreeToRadian<float>(cloudRot), degreeToRadian<float>(0));

#ifdef SSU_DEBUG_DISPLAY_GYRO
	  if (objGyro) {
		objGyro->SetRotation(vecGyro.x, vecGyro.y, vecGyro.z);
		const Vector3F pos(eyePos.x, eyePos.y, eyePos.z);
		const Vector3F front(eyeDir * 5.0f);
		const Vector3F right(eyeDir.Cross(Vector3F(0, -1, 0)).Normalize() * 1.0f);
		const Vector3F up(front.Cross(right).Normalize() * -1.0f);
		objGyro->SetTranslation(pos + front + right + up);
	  }
#endif // SSU_DEBUG_DISPLAY_GYRO

	  return SCENEID_CONTINUE;
	}

	/**
	*/
	int DoTransitionToLevelSelect(Engine&, float) {
	  return SCENEID_CONTINUE;
	}

	/**
	*/
	int DoLevelSelect(Engine&, float) {
	  return SCENEID_CONTINUE;
	}

	/**
	*/
	int DoTransitionToRecordViwer(Engine&, float) {
	  return SCENEID_CONTINUE;
	}

	/** Do fade out.
	
	Transition to the scene of the start event when the fadeout finished.
	This is the update function called from Update().
	*/
	int DoFadeOut(Engine& engine, float) {
	  Renderer& r = engine.GetRenderer();
	  if (r.GetCurrentFilterMode() == Renderer::FILTERMODE_NONE) {
		r.FadeIn(1.0f);
		engine.GetAudio().StopBGM();
		return SCENEID_STARTEVENT;
	  }
	  return SCENEID_CONTINUE;
	}

	virtual void ProcessWindowEvent(Engine& engine, const Event& e) {
	  Renderer& r = engine.GetRenderer();
	  if (r.GetCurrentFilterMode() == Renderer::FILTERMODE_NONE) {
		switch (e.Type) {
		case Event::EVENT_MOUSE_BUTTON_PRESSED:
		  if (e.MouseButton.Button == MOUSEBUTTON_LEFT) {
			r.FadeOut(Color4B(0, 0, 0, 0), 1.0f);
			updateFunc = &TitleScene::DoFadeOut;
		  }
		  break;
#ifndef NDEBUG
		case Event::EVENT_KEY_PRESSED:
		  switch (e.Key.Code) {
		  case KEY_A:
			eyeDir = (Matrix4x4::RotationY(degreeToRadian(-1.0f)) * eyeDir).ToVec3();
			break;
		  case KEY_D:
			eyeDir = (Matrix4x4::RotationY(degreeToRadian(1.0f)) * eyeDir).ToVec3();
			break;
		  case KEY_W:
			eyeDir = (Quaternion(eyeDir.Cross(Vector3F(0, 1, 0)), degreeToRadian(1.0f)).Apply(eyeDir));
			break;
		  case KEY_S:
			eyeDir = (Quaternion(eyeDir.Cross(Vector3F(0, 1, 0)), degreeToRadian(-1.0f)).Apply(eyeDir));
			break;
		  case KEY_UP:
			eyePos.y += 1.0f;
			break;
		  case KEY_DOWN:
			eyePos.y -= 1.0f;
			break;
		  case KEY_LEFT:
			eyePos -= eyeDir.Cross(Vector3F(0, 1, 0)).Normalize();
			break;
		  case KEY_RIGHT:
			eyePos += eyeDir.Cross(Vector3F(0, 1, 0)).Normalize();
			break;
		  case KEY_SPACE: {
			static const char* const animeNameList[] = {
			  "Stand", "Wait0", "Wait1", "Walk", "Dive"
			};
			animeNo = (animeNo + 1) % 5;
			objList[1]->SetAnimation(engine.GetRenderer().GetAnimation(animeNameList[animeNo]));
			break;
		  }
		  }
		  break;
#ifdef SSU_DEBUG_DISPLAY_GYRO
		case Event::EVENT_GYRO:
		  vecGyro = Vector3F(e.Gyro.X, e.Gyro.Y, e.Gyro.Z).Normalize();
		  break;
#endif // SSU_DEBUG_DISPLAY_GYRO
#endif // NDEBUG
		default:
		  break;
		}
	  }
	}

	virtual void Draw(Engine& engine) {
	  Renderer& r = engine.GetRenderer();
	  if (r.GetCurrentFilterMode() == Renderer::FILTERMODE_NONE) {
		if (pTouchMeItem) {
		  pTouchMeItem->Draw(r, Vector2F(0, 0));
		}
	  }
	  r.Render(&objList[0], &objList[0] + objList.size());
	}

  private:
	std::vector<ObjectPtr> objList;
	Position3F eyePos;
	Vector3F eyeDir;
	int animeNo;
	bool loaded;

	MenuItem::Pointer pTouchMeItem;
	Menu levelMenu;

	float cloudRot;
	int (TitleScene::*updateFunc)(Engine&, float);

#ifdef SSU_DEBUG_DISPLAY_GYRO
	Vector3F vecGyro;
	ObjectPtr  objGyro;
#endif // SSU_DEBUG_DISPLAY_GYRO
  };

  ScenePtr CreateTitleScene(Engine&) {
	return ScenePtr(new TitleScene());
  }

} // namespace SunnySideUp
