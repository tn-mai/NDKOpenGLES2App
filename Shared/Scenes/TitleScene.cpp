#include "Scene.h"
#include "SaveData.h"
#include "../../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include <vector>
#include <memory>
#include <functional>
#include <boost/math/constants/constants.hpp>

#ifndef NDEBUG
//#define SSU_DEBUG_DISPLAY_GYRO
#endif // NDEBUG

namespace SunnySideUp {

  using namespace Mai;

  Vector2F GetDeviceIndependentPositon(int x, int y, int w, int h) {
	return Vector2F(static_cast<float>(x) / static_cast<float>(w), static_cast<float>(y) / static_cast<float>(h));
  }

  /** The base class of menu items.

    All menu item classes should inherit this class.
  */
  struct MenuItem {
	typedef std::shared_ptr<MenuItem> Pointer;
	typedef std::function<bool(const Vector2F&, MouseButton)> ClickEventHandler;
	typedef std::function<bool(const Vector2F&, const Vector2F&)> MoveEventHandler;

	MenuItem() : lt(0, 0), wh(0, 0), isActive(true){}
	virtual ~MenuItem() = 0;
	virtual void Draw(Renderer&, Vector2F, float) const {}
	virtual void Update(float) {}

	virtual bool OnClick(const Vector2F& currentPos, MouseButton button) {
	  if (clickHandler) {
		return clickHandler(currentPos, button);
	  }
	  return false;
	}
	virtual bool OnMouseMove(const Vector2F& currentPos, const Vector2F& startPos) {
	  if (mouseMoveHandler) {
		return mouseMoveHandler(currentPos, startPos);
	  }
	  return false;
	}

	void SetRegion(const Vector2F& pos, const Vector2F& size) {
	  lt = pos;
	  wh = size;
	}
	bool OnRegion(const Vector2F& pos) const {
	  if (!isActive) {
		return false;
	  }
	  if (pos.x < lt.x || pos.x > lt.x + wh.x) {
		return false;
	  } else if (pos.y < lt.y || pos.y > lt.y + wh.y) {
		return false;
	  }
	  return true;
	}
	float GetAlpha() const { return isActive ? 1.0f : 0.25f };

	Vector2F lt, wh; ///< Active area.
	ClickEventHandler clickHandler;
	MoveEventHandler mouseMoveHandler;
	bool isActive;
  };
  inline MenuItem::~MenuItem() = default;

  /** A text menu item.
  */
  struct TextMenuItem : public MenuItem {
	TextMenuItem(const char* str, const Vector2F& p, float s) : pos(p), baseScale(s), scaleTick(0), flags(0) {
	  const size_t len = std::min(sizeof(label) - 1, strlen(str));
	  std::copy(str, str + len, label);
	  label[len] = '\0';
	  wh = Vector2F(static_cast<float>(len) * 0.1f * baseScale, baseScale * 0.1f);
	  lt = pos - wh * 0.5f;
	  color = Color4B(240, 240, 240, static_cast<uint8_t>(255.0f * s));
	}
	virtual ~TextMenuItem() {}
	virtual void Draw(Renderer& r, Vector2F offset, float alpha) const {
	  offset += pos;
	  float scale;
	  if (scaleTick < 1.0f) {
		scale = 1.0f + scaleTick * 0.5f;
	  } else {
		scale = 1.5f - (scaleTick - 1.0f) * 0.5f;
	  }
	  scale *= baseScale;
	  const float w = r.GetStringWidth(label) * scale * 0.5f;
	  Color4B c = color;
	  c.a = static_cast<uint8_t>(c.a * alpha * GetAlpha());
	  r.AddString(offset.x - w, offset.y, scale, c, label);
	}
	virtual void Update(float tick) {
	  if (flags & FLAG_ZOOM_ANIMATION) {
		scaleTick += tick;
		if (scaleTick > 2.0f) {
		  scaleTick -= 2.0f;
		}
	  }
	}

	int GetFlag() const { return flags; }
	void SetFlag(int f) { flags |= f; }
	void ClearFlag(int f) {
	  flags &= ~f;
	  if (!(flags & FLAG_ZOOM_ANIMATION)) {
		scaleTick = 0.0f;
	  }
	}

	static const uint8_t FLAG_ZOOM_ANIMATION = 0x01;
	static const uint8_t FLAG_ALPHA_ANIMATION = 0x02;

	Vector2F pos;
	Color4B color;
	float baseScale;
	float scaleTick;
	uint8_t flags;
	char label[15];
  };

  /** Menu container.

    Usually, this is used as the root of menu.
  */
  struct Menu : public MenuItem {
	Menu() : pos(0, 0) {
	  SetRegion(Vector2F(0, 0), Vector2F(1, 1));
	}
	virtual ~Menu() {}
	virtual void Draw(Renderer& r, Vector2F offset, float alpha) const {
	  offset += pos;
	  alpha *= GetAlpha();
	  for (auto& e : items) {
		e->Draw(r, offset, alpha);
	  }
	}
	virtual void Update(float tick) {
	  for (auto& e : items) {
		e->Update(tick);
	  }
	}
	virtual bool OnClick(const Vector2F& currentPos, MouseButton button) {
	  const auto re = items.rend();
	  for (auto ri = items.rbegin(); ri != re; ++ri) {
		if ((*ri)->OnRegion(currentPos)) {
		  return (*ri)->OnClick(currentPos, button);
		}
	  }
	  return false;
	}
	virtual bool OnMouseMove(const Vector2F& currentPos, const Vector2F& startPos) {
	  const auto re = items.rend();
	  for (auto ri = items.rbegin(); ri != re; ++ri) {
		if ((*ri)->OnRegion(currentPos)) {
		  return (*ri)->OnMouseMove(currentPos, startPos);
		}
	  }
	  return false;
	}

	bool ProcessWindowEvent(Engine& engine, const Event& e) {
	  const int windowWidth = engine.GetWindow().GetWidth();
	  const int windowHeight = engine.GetWindow().GetHeight();
	  switch (e.Type) {
	  case Event::EVENT_MOUSE_MOVED:
		if (pActiveItem) {
		  const Vector2F currentPos = GetDeviceIndependentPositon(e.MouseMove.X, e.MouseMove.Y, windowWidth, windowHeight);
		  return pActiveItem->OnMouseMove(currentPos, dragStartPoint);
		}
		break;
	  case Event::EVENT_MOUSE_BUTTON_PRESSED: {
		const Vector2F currentPos = GetDeviceIndependentPositon(e.MouseButton.X, e.MouseButton.Y, windowWidth, windowHeight);
		if (OnRegion(currentPos)) {
		  dragStartPoint = currentPos;
		  const auto re = items.rend();
		  for (auto ri = items.rbegin(); ri != re; ++ri) {
			if ((*ri)->OnRegion(currentPos)) {
			  pActiveItem = *ri;
			  return true;
			}
		  }
		}
		break;
	  }
	  case Event::EVENT_MOUSE_BUTTON_RELEASED: {
		const Vector2F currentPos = GetDeviceIndependentPositon(e.MouseButton.X, e.MouseButton.Y, windowWidth, windowHeight);
		if (pActiveItem) {
		  MenuItem::Pointer p(std::move(pActiveItem));
		  if (p->OnClick(currentPos, e.MouseButton.Button)) {
			return true;
		  }
		}
		if (OnRegion(currentPos)) {
		  const auto re = items.rend();
		  for (auto ri = items.rbegin(); ri != re; ++ri) {
			if ((*ri)->OnRegion(currentPos)) {
			  return (*ri)->OnClick(currentPos, e.MouseButton.Button);
			}
		  }
		}
		break;
	  }
	  }
	  return false;
	}

	void Add(MenuItem::Pointer p) { items.push_back(p); }
	void Clear() { items.clear(); }

	Vector2F pos;
	std::vector<MenuItem::Pointer> items;
	Vector2F dragStartPoint;
	MenuItem::Pointer pActiveItem;
  };

  /** Multiple menu item container.
  */
  struct CarouselMenu : public MenuItem {
	CarouselMenu(const Vector2F& p, int size, int top)
	  : pos(p), windowSize(size), topOfWindow(top), hasDragging(false), moveY(0)
	{
	  SetRegion(Vector2F(0, 0), Vector2F(1, 1));
	}
	virtual ~CarouselMenu() {}
	virtual void Draw(Renderer& r, Vector2F offset, float alpha) const {
	  offset += pos;
	  alpha *= GetAlpha();
	  for (auto& e : renderingList) {
		e->Draw(r, offset, alpha);
	  }
	}
	virtual void Update(float tick) {

	  if (!hasDragging) {
		if (moveY < 0.0f) {
		  moveY = std::min(0.0f, moveY + tick * 0.5f);
		} else if (moveY > 0.0f) {
		  moveY = std::max(0.0f, moveY - tick * 0.5f);
		}
	  }
	  const int containerSize = static_cast<int>(items.size());
	  const float center = static_cast<float>(windowSize / 2);
	  const float unitTheta = boost::math::constants::pi<float>() * 0.5f / static_cast<float>(windowSize);
	  const float startPos = moveY * 10.0f;
	  const int indexOffset = static_cast<int>(startPos);
	  const float fract = startPos - static_cast<float>(indexOffset);
	  renderingList.clear();
	  for (int i = 0; i < windowSize; ++i) {
		const float currentPos = startPos + static_cast<float>(topOfWindow + i);
		const float theta = (static_cast<float>(i) - center + fract) * unitTheta;
		const float alpha = std::cos(theta);
		const int index = topOfWindow + i - indexOffset;
		std::shared_ptr<TextMenuItem> pItem = std::static_pointer_cast<TextMenuItem>(items[(index + containerSize) % containerSize]);
		if (i != center) {
		  pItem->color = Color4B(200, 200, 180, static_cast<uint8_t>(255.0f * alpha * alpha * alpha));
		} else {
		  pItem->color = Color4B(240, 240, 240, 255);
		}
		pItem->baseScale = (alpha * alpha) * 1.5f;
		pItem->pos.y = std::sin(theta) * 0.25f;
		renderingList.push_back(pItem);
	  }
	  std::sort(
		renderingList.begin(),
		renderingList.end(),
		[](const MenuItem::Pointer& lhs, const MenuItem::Pointer& rhs) {
		  return static_cast<const TextMenuItem*>(lhs.get())->baseScale > static_cast<const TextMenuItem*>(rhs.get())->baseScale;
		}
	  );

	  for (auto& e : items) {
		e->Update(tick);
	  }
	}
	virtual bool OnClick(const Vector2F& currentPos, MouseButton button) {
	  const int containerSize = static_cast<int>(items.size());
	  if (!hasDragging) {
		const int center = windowSize / 2;
		return items[(topOfWindow + center) % containerSize]->OnClick(currentPos, button);
	  }
	  topOfWindow = (topOfWindow - static_cast<int>(moveY * 10.0f) + containerSize) % containerSize;
	  hasDragging = false;
	  moveY *= 10.0f;
	  moveY -= static_cast<float>(static_cast<int>(moveY));
	  moveY *= 0.1f;
	  return true;
	}
	virtual bool OnMouseMove(const Vector2F& currentPos, const Vector2F& dragStartPoint) {
	  moveY = currentPos.y - dragStartPoint.y;
	  hasDragging = true;
	  return true;
	}

	void Add(MenuItem::Pointer p) { items.push_back(p); }
	void Clear() {
	  renderingList.clear();
	  items.clear();
	}

	Vector2F pos;
	std::vector<MenuItem::Pointer> items;
	std::vector<MenuItem::Pointer> renderingList;
	int windowSize;
	int topOfWindow;
	float moveY;
	bool hasDragging;
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

		std::shared_ptr<TextMenuItem> pTouchMeItem(new TextMenuItem("TOUCH ME!", Vector2F(0.5f, 0.7f), 1.0f));
		pTouchMeItem->SetFlag(TextMenuItem::FLAG_ZOOM_ANIMATION);
		pTouchMeItem->SetRegion(Vector2F(0, 0), Vector2F(1, 1));
		pTouchMeItem->clickHandler = [this, &r](const Vector2F&, MouseButton) -> bool {
		  if (r.GetCurrentFilterMode() != Renderer::FILTERMODE_NONE) {
			return false;
		  }
		  rootMenu.Clear();
		  std::shared_ptr<CarouselMenu> pCarouselMenu(new CarouselMenu(Vector2F(0.5f, 0.5f), 5, 6));
		  for (int i = 0; i < 8; ++i) {
			std::ostringstream ss;
			ss << "LEVEL " << (i + 1);
			MenuItem::Pointer pItem(new TextMenuItem(ss.str().c_str(), Vector2F(0, 0), 1.0f));
			pItem->clickHandler = [this, &r, i](const Vector2F&, MouseButton) -> bool {
			  selectedLevel = i;
			  r.FadeOut(Color4B(0, 0, 0, 0), 1.0f);
			  updateFunc = &TitleScene::DoFadeOut;
			  return true;
			};
			pCarouselMenu->Add(pItem);
		  }
		  MenuItem::Pointer pRecordItem(new TextMenuItem("RECORD", Vector2F(0.25f, 0.9f), 1.0f));
		  pRecordItem->clickHandler = [this, pCarouselMenu](const Vector2F&, MouseButton) -> bool {
			isActive = false;
			pCarouselMenu->isActive = false;
			return true;
		  };
		  rootMenu.Add(pCarouselMenu);
		  rootMenu.Add(pRecordItem);
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
	  rootMenu.Update(tick);
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
		engine.GetCommonData<CommonData>()->level = selectedLevel;
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
		switch (e.Type) {
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

	Menu rootMenu;
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
