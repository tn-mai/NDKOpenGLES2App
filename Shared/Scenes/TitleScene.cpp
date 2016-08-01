#include "Scene.h"
#include "../Menu.h"
#include "SaveData.h"
#include "LevelInfo.h"
#include "../../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include <vector>

#ifndef NDEBUG
//#define SSU_DEBUG_DISPLAY_GYRO
//#define SSU_ENABLE_LANDSCAPE_CHEKING_MODE
#endif // NDEBUG

#ifdef SSU_ENABLE_LANDSCAPE_CHEKING_MODE
#include "../File.h"
#endif // SSU_ENABLE_LANDSCAPE_CHEKING_MODE

namespace SunnySideUp {

  using namespace Mai;

#ifdef SSU_ENABLE_LANDSCAPE_CHEKING_MODE
  static Vector3F shadowOffset(-200.0f, 0.0f, -200.0f);
#endif // SSU_ENABLE_LANDSCAPE_CHEKING_MODE

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

#ifdef SSU_ENABLE_LANDSCAPE_CHEKING_MODE
		if (auto pBuf = FileSystem::LoadFile("Meshes/CoastTownSpace.msh")) {
		  const auto result = Mesh::ImportGeometry(*pBuf);
		  if (result.result == Mesh::Result::success && !result.geometryList.empty()) {
			const Mesh::Geometry& m = result.geometryList[0];
			static const char* const meshNameList[] = {
			  "CityBlock00",
			  "CityBlock01",
			  "CityBlock02",
			  "CityBlock03",
			  "CityBlock04",
			  "CityBlock05",
			  "CityBlock06",
			  "CityBlock07",
			};
			static const Color4B colorList[] = {
			  Color4B(200, 200, 200, 255),
			  Color4B(255, 200, 200, 255),
			  Color4B(200, 255, 200, 255),
			  Color4B(200, 200, 255, 255),
			  Color4B(255, 255, 200, 255),
			};
			int i = 0;
			for (const auto& e : m.vertexList) {
			  auto obj = r.CreateObject(
				meshNameList[i % (sizeof(meshNameList) / sizeof(meshNameList[0]))],
				Material(colorList[i % (sizeof(colorList) / sizeof(colorList[0]))], 0, 0),
				"default"
			  );
			  obj->SetScale(Vector3F(4, 4, 4));
			  obj->SetTranslation(Vector3F(e.position.x * 12.0f, e.position.y * 12.0f, e.position.z * 12.0f));
			  const float ryRandomFactor = std::uniform_real_distribution<float>(0.0f, 0.2f)(random);
			  const float ry = std::asin(e.tangent.z / e.tangent.x);// +ryRandomFactor;
			  obj->SetRotation(degreeToRadian<float>(0), ry, degreeToRadian<float>(0));
			  objList.push_back(obj);
			  ++i;
			}
		  }
		}
		{
		  auto obj = r.CreateObject("LandScape.Coast", Material(Color4B(200, 200, 200, 255), 0, 0), "solidmodel");
		  obj->SetScale(Vector3F(12, 12, 12));
		  objList.push_back(obj);
		}
		{
		  auto obj = r.CreateObject("LandScape.Coast.Levee", Material(Color4B(255, 255, 255, 255), 0, 0), "default");
		  obj->SetScale(Vector3F(12, 12, 12));
		  objList.push_back(obj);
		}
		{
		  auto obj = r.CreateObject("LandScape.Coast.Flora", Material(Color4B(200, 200, 200, 255), 0, 0), "defaultWithAlpha");
		  obj->SetScale(Vector3F(12, 12, 12));
		  objList.push_back(obj);
		}
		{
		  auto obj = r.CreateObject("LandScape.Coast.Ships", Material(Color4B(200, 200, 200, 255), 0, 0), "defaultWithAlpha");
		  obj->SetScale(Vector3F(12, 12, 12));
		  objList.push_back(obj);
		}
		{
		  auto obj = r.CreateObject("tower00", Material(Color4B(200, 200, 200, 255), 0, 0), "default");
		  obj->SetScale(Vector3F(4, 4, 4));
		  obj->SetTranslation(Vector3F(-360, 18.5f, -550));
		  obj->SetRotation(degreeToRadian<float>(0), degreeToRadian<float>(0), degreeToRadian<float>(0));
		  objList.push_back(obj);
		}
#endif // SSU_ENABLE_LANDSCAPE_CHEKING_MODE

#ifdef SSU_DEBUG_DISPLAY_GYRO
		vecGyro = Vector3F::Unit();
		objGyro = r.CreateObject("unitbox", Material(Color4B(255, 255, 255, 224), 0, 0), "default");
		objGyro->SetScale(Vector3F(1, 0.25f, 0.5f));
		objList.push_back(objGyro);
#endif // SSU_DEBUG_DISPLAY_GYRO

		const Vector3F shadowDir = GetSunRayDirection(r.GetTimeOfScene());
		r.SetShadowLight(objList[0]->Position() - shadowDir * 200.0f, shadowDir, 100, 250, Vector2F(10, 8 * 4));

		pRecordView.reset(new Menu::Menu());
		pRecordView->Add(
		  Menu::MenuItem::Pointer(new Menu::TextMenuItem("BEST RECORDS", Vector2F(0.5f, 0.05f), 1.25f, Color4B(255, 240, 32, 255)))
		);
		{
		  const int levelCount = GetMaximumLevel() + 1;
		  std::shared_ptr<Menu::SwipableMenu> pSwipableView(new Menu::SwipableMenu(levelCount));
		  pSwipableView->SetRegion(Vector2F(0.0f, 0.0f), Vector2F(1.0f, 0.8f));
		  pRecordView->Add(pSwipableView);
		  for (int i = 0; i < levelCount; ++i) {
			char  buf[32];
			snprintf(buf, 32, "LEVEL %d", i + 1);
			pSwipableView->Add(i, Menu::MenuItem::Pointer(new Menu::TextMenuItem(buf, Vector2F(0.5f, 0.175f), 1.0f)));
			const int courseCount = GetMaximumCourseNo(i) + 1;
			for (int j = 0; j < courseCount; ++j) {
			  if (auto e = SaveData::GetBestRecord(i, j)) {
				const int msec = static_cast<int>(e->time % 1000);
				const int min = static_cast<int>(e->time / 1000 / 60);
				const int sec = static_cast<int>((e->time / 1000) % 60);
				snprintf(buf, 32, "%d-%d %02d:%02d.%03d", i + 1, j + 1, min, sec, msec);
			  } else {
				snprintf(buf, 32, "%d-%d --:--.---", i + 1, j + 1);
			  }
			  pSwipableView->Add(i, Menu::MenuItem::Pointer(new Menu::TextMenuItem(buf, Vector2F(0.5f, 0.275f + static_cast<float>(j) * 0.075f), 1.0f)));
			}
		  }

		  std::shared_ptr<Menu::TextMenuItem> pReturnItem(new Menu::TextMenuItem("RETURN", Vector2F(0.25f, 0.9f), 1.0f));
		  pReturnItem->color = Color4B(240, 64, 32, 255);
		  pReturnItem->clickHandler = [this](const Vector2F&, MouseButton) -> bool {
			rootMenu.Clear();
			rootMenu.Add(pLevelSelect);
			rootMenu.inputDisableTimer = 0.25f;
			return true;
		  };
		  pRecordView->Add(pReturnItem);

		  std::shared_ptr<Menu::TextMenuItem> pClearItem(new Menu::TextMenuItem("CLEAR", Vector2F(0.75f, 0.9f), 1.0f));
		  pClearItem->color = Color4B(32, 64, 240, 255);
		  pClearItem->clickHandler = [this, &engine](const Vector2F&, MouseButton) -> bool {
			std::shared_ptr<Menu::TextMenuItem> pLabelItem(new Menu::TextMenuItem("CLARE RECORD?", Vector2F(0.5f, 0.25f), 1.0f));
			pLabelItem->color = Color4B(32, 64, 240, 255);
			std::shared_ptr<Menu::TextMenuItem> pYesItem(new Menu::TextMenuItem(" YES ", Vector2F(0.5f, 0.4f), 2.0f));
			pYesItem->clickHandler = [this, &engine](const Vector2F&, MouseButton) -> bool {
			  SaveData::DeleteAll(engine.GetWindow());
			  char  buf[] = "0-0 --:--.---";
			  std::shared_ptr<Menu::SwipableMenu> pSwipableMenu = std::static_pointer_cast<Menu::SwipableMenu>(pRecordView->items[1]);
			  const size_t viewCount = pSwipableMenu->ViewCount();
			  for (int i = 0; i < viewCount; ++i) {
				buf[0] = '1' + i;
				const size_t itemCount = pSwipableMenu->ItemCount(i);
				for (int j = 1; j < itemCount; ++j) {
				  buf[2] = '0' + j;
				  std::shared_ptr<Menu::TextMenuItem> p = std::static_pointer_cast<Menu::TextMenuItem>(pSwipableMenu->GetItem(i, j));
				  p->SetText(buf);
				}
			  }
			  std::shared_ptr<Menu::TextMenuItem> pLabel0Item(new Menu::TextMenuItem("CLEAR", Vector2F(0.5f, 0.45f), 1.0f));
			  std::shared_ptr<Menu::TextMenuItem> pLabel1Item(new Menu::TextMenuItem("ALL RECORDS", Vector2F(0.5f, 0.55f), 1.0f));
			  pLabel1Item->SetRegion(Vector2F(0, 0), Vector2F(1, 1));
			  pLabel1Item->clickHandler = [this](const Vector2F&, MouseButton) -> bool {
				rootMenu.Clear();
				rootMenu.Add(pRecordView);
				rootMenu.inputDisableTimer = 0.25f;
				return true;
			  };
			  rootMenu.Clear();
			  rootMenu.Add(pLabel0Item);
			  rootMenu.Add(pLabel1Item);
			  rootMenu.inputDisableTimer = 0.25f;
			  return true;
			};
			std::shared_ptr<Menu::TextMenuItem> pNoItem(new Menu::TextMenuItem(" NO ", Vector2F(0.5f, 0.6f), 2.0f));
			pNoItem->clickHandler = [this](const Vector2F&, MouseButton) -> bool {
			  rootMenu.Clear();
			  rootMenu.Add(pRecordView);
			  rootMenu.inputDisableTimer = 0.25f;
			  return true;
			};

			rootMenu.Clear();
			rootMenu.Add(pLabelItem);
			rootMenu.Add(pYesItem);
			rootMenu.Add(pNoItem);
			rootMenu.inputDisableTimer = 0.25f;
			return true;
		  };
		  pRecordView->Add(pClearItem);
		}

		pLevelSelect.reset(new Menu::Menu());
		{
		  std::shared_ptr<Menu::TextMenuItem> pTitleLabel(new Menu::TextMenuItem("SELECT LEVEL", Vector2F(0.5f, 0.1f), 1.25f));
		  pTitleLabel->color = Color4B(250, 192, 128, 255);
		  pLevelSelect->Add(pTitleLabel);

		  std::shared_ptr<Menu::CarouselMenu> pCarouselMenu(new Menu::CarouselMenu(Vector2F(0.5f, 0.4f), 5, 6, 1.75f));
		  for (int i = 0; i < 8; ++i) {
			std::ostringstream ss;
			ss << "LEVEL " << (i + 1);
			Menu::MenuItem::Pointer pItem(new Menu::TextMenuItem(ss.str().c_str(), Vector2F(0, 0), 1.0f));
			pItem->clickHandler = [this, &r, i](const Vector2F&, MouseButton) -> bool {
			  selectedLevel = i;
			  r.FadeOut(Color4B(0, 0, 0, 0), 1.0f);
			  updateFunc = &TitleScene::DoFadeOut;
			  return true;
			};
			pCarouselMenu->Add(pItem);
		  }
		  pLevelSelect->Add(pCarouselMenu);

		  std::shared_ptr<Menu::TextMenuItem> pRecordItem(new Menu::TextMenuItem("RECORD", Vector2F(0.25f, 0.9f), 1.0f));
		  pRecordItem->color = Color4B(240, 64, 32, 255);
		  pRecordItem->clickHandler = [this](const Vector2F&, MouseButton) -> bool {
			rootMenu.Clear();
			rootMenu.Add(pRecordView);
			rootMenu.inputDisableTimer = 0.25f;
			return true;
		  };
		  pLevelSelect->Add(pRecordItem);
		}

		std::shared_ptr<Menu::TextMenuItem> pTouchMeItem(new Menu::TextMenuItem("TOUCH ME!", Vector2F(0.5f, 0.7f), 1.0f, Menu::TextMenuItem::FLAG_ZOOM_ANIMATION));
		pTouchMeItem->SetRegion(Vector2F(0, 0), Vector2F(1, 1));
		pTouchMeItem->clickHandler = [this, &r](const Vector2F&, MouseButton) -> bool {
		  if (r.GetCurrentFilterMode() != Renderer::FILTERMODE_NONE) {
			return false;
		  }
		  rootMenu.Clear();
		  rootMenu.Add(pLevelSelect);
		  rootMenu.inputDisableTimer = 0.25f;
		  return true;
		};
		rootMenu.Add(pTouchMeItem);

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
		rootMenu.Clear();
		loaded = false;
	  }
	  status = STATUSCODE_STOPPED;
	  return true;
	}

	virtual int Update(Engine& engine, float tick) {
	  for (auto e : objList) {
		e->Update(tick);
	  }
	  rootMenu.Update(engine, tick);
	  Renderer& r = engine.GetRenderer();
	  r.Update(tick, eyePos, eyeDir, Vector3F(0, 1, 0));

#ifdef SSU_ENABLE_LANDSCAPE_CHEKING_MODE
	  const Vector3F shadowDir = GetSunRayDirection(r.GetTimeOfScene());
	  r.SetShadowLight(objList[0]->Position() - shadowDir * 150.0f + shadowOffset, shadowDir, 100, 400, Vector2F(0.5f, 0.25f));
#endif // SSU_ENABLE_LANDSCAPE_CHEKING_MODE

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
		CommonData& commonData = *engine.GetCommonData<CommonData>();
		commonData.level = selectedLevel;
		commonData.courseNo = 0;
		commonData.remainingEggs = sizeOfEggPack;
		r.FadeIn(1.0f);
		engine.GetAudio().StopBGM();
		return SCENEID_STARTEVENT;
	  }
	  return SCENEID_CONTINUE;
	}

	virtual void ProcessWindowEvent(Engine& engine, const Event& e) {
	  if (rootMenu.ProcessWindowEvent(engine, e)) {
		return;
	  }

	  Renderer& r = engine.GetRenderer();
	  if (r.GetCurrentFilterMode() == Renderer::FILTERMODE_NONE) {
		static const float moveSpeed = 10.0f;
		switch (e.Type) {
#ifndef NDEBUG
		case Event::EVENT_KEY_PRESSED:
		  switch (e.Key.Code) {
#ifdef SSU_ENABLE_LANDSCAPE_CHEKING_MODE
		  case KEY_Z:
			shadowOffset.x += 100;
			break;
		  case KEY_X:
			shadowOffset.x -= 100;
			break;
		  case KEY_C:
			shadowOffset.z += 100;
			break;
		  case KEY_V:
			shadowOffset.z -= 100;
			break;
#endif // SSU_ENABLE_LANDSCAPE_CHEKING_MODE
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
			eyePos.y += moveSpeed;
			break;
		  case KEY_DOWN:
			eyePos.y -= moveSpeed;
			break;
		  case KEY_R:
			eyePos += eyeDir * moveSpeed;
			break;
		  case KEY_F:
			eyePos -= eyeDir * moveSpeed;
			break;
		  case KEY_LEFT:
			eyePos -= eyeDir.Cross(Vector3F(0, 1, 0)).Normalize() * moveSpeed;
			break;
		  case KEY_RIGHT:
			eyePos += eyeDir.Cross(Vector3F(0, 1, 0)).Normalize() * moveSpeed;
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
		rootMenu.Draw(r, Vector2F(0, 0), 1.0f);
	  }
	  r.Render(&objList[0], &objList[0] + objList.size());
	}

  private:
	std::vector<ObjectPtr> objList;
	Position3F eyePos;
	Vector3F eyeDir;
	int animeNo;
	bool loaded;

	Menu::Menu rootMenu;
	std::shared_ptr<Menu::Menu> pRecordView;
	std::shared_ptr<Menu::Menu> pLevelSelect;
	int selectedLevel;

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
