/** @file Menu.h
*/
#ifndef MAI_COMMON_MENU_H_INCLUDED
#define MAI_COMMON_MENU_H_INCLUDED
#include "Vector.h"
#include "Mouse.h"
#include <vector>
#include <memory>
#include <functional>

namespace Mai {

class Engine;
class Renderer;
class Event;

/** Simple menu system.

  It provides the event-driven response to user input.
*/
namespace Menu {

  extern const char SEID_PageFeed[];
  extern const char SEID_Carousel[];
  extern const char SEID_Confirm[];
  extern const char SEID_Cancel[];

  void Initialize(Engine&);
  void Finalize(Engine&);

  /** A moving state of mouse.
  */
  enum class MouseMoveState {
	Begin, ///< Begin mouse cursor moving.
	Moving, ///< Mouse cursor continues to move.
	End, ///< End mouse cursor moving.
  };

  /** Button action type.
  */
  enum class ButtonAction {
	Down,
	Up,
  };

  /** The base class of any menu item.
  */
  struct MenuItem {
	typedef std::shared_ptr<MenuItem> Pointer;
	typedef std::function<bool(const Vector2F&, MouseButton)> ClickEventHandler;
	typedef std::function<bool(const Vector2F&, const Vector2F&, MouseMoveState)> MoveEventHandler;
	typedef std::function<bool(const Vector2F&, MouseButton, ButtonAction)> ButtonEventHandler;

	MenuItem();
	virtual ~MenuItem() = 0;
	virtual void Draw(Renderer&, Vector2F, float) const {}
	virtual void Update(Engine&, float) {}
	virtual bool OnClick(Engine& engine, const Vector2F& currentPos, MouseButton button);
	virtual bool OnMouseButtonDown(Engine& engine, const Vector2F& currentPos, MouseButton button);
	virtual bool OnMouseButtonUp(Engine& engine, const Vector2F& currentPos, MouseButton button);
	virtual bool OnMouseMove(Engine& engine, const Vector2F& currentPos, const Vector2F& startPos, MouseMoveState);

	void SetRegion(const Vector2F& pos, const Vector2F& size);
	bool OnRegion(const Vector2F& pos) const;
	float GetAlpha() const;

	Vector2F lt, wh; ///< Active area.
	ClickEventHandler clickHandler;
	MoveEventHandler mouseMoveHandler;
	ButtonEventHandler buttonHandler;
	bool isActive;
  };
  inline MenuItem::~MenuItem() = default;

  /** A text menu item.
  */
  struct TextMenuItem : public MenuItem {
	static const uint8_t FLAG_ZOOM_ANIMATION = 0x01;
	static const uint8_t FLAG_ALPHA_ANIMATION = 0x02;
	static const uint8_t FLAG_SHADOW = 0x04;
	static const uint8_t FLAG_OUTLINE = 0x08;
	static const uint8_t FLAG_BUTTON = 0x10;
	static const uint8_t FLAG_BUTTON_DOWN = 0x20;

	TextMenuItem(const char* str, const Vector2F& p, float s, int flg = FLAG_SHADOW);
	TextMenuItem(const char* str, const Vector2F& p, float s, Color4B c, int flg = FLAG_SHADOW);
	virtual ~TextMenuItem() {}
	virtual void Draw(Renderer& r, Vector2F offset, float alpha) const;
	virtual void Update(Engine&, float tick);
	virtual bool OnMouseButtonDown(Engine& engine, const Vector2F& currentPos, MouseButton button);
	virtual bool OnMouseButtonUp(Engine& engine, const Vector2F& currentPos, MouseButton button);
	virtual bool OnClick(Engine& engine, const Vector2F& currentPos, MouseButton button);

	void SetText(const char*);
	const char* GetText() const;
	int GetFlag() const;
	void SetFlag(int f);
	void ClearFlag(int f);

	Vector2F pos;
	Color4B color;
	float baseScale;
	float scaleTick;
	uint8_t flags;
	char label[15];
  };

  /** Multiple text menu item container.

    This class has virtucal carousel user interface.
  */
  struct CarouselMenu : public MenuItem {
	typedef std::function<void(int, int)> ChangeEventHandler;

	CarouselMenu(const Vector2F& p, int size, int top, float s);
	virtual ~CarouselMenu() {}
	virtual void Draw(Renderer& r, Vector2F offset, float alpha) const;
	virtual void Update(Engine& engine, float tick);
	virtual bool OnClick(Engine& engine, const Vector2F& currentPos, MouseButton button);
	virtual bool OnMouseMove(Engine& engine, const Vector2F& currentPos, const Vector2F& dragStartPoint, MouseMoveState);

	void Add(MenuItem::Pointer p);
	void Clear();

	Vector2F pos;
	std::vector<MenuItem::Pointer> items;
	std::vector<MenuItem::Pointer> renderingList;
	int windowSize;
	int topOfWindow;
	int prevOffset;
	float scale;
	float moveY;
	bool hasDragging;
	ChangeEventHandler changeEventHandler;
  };

  /** Swipable menu.
  */
  struct SwipableMenu : public MenuItem {
	typedef std::function<void(int, int)> SwipeEventHandler;

	explicit SwipableMenu(size_t);
	virtual ~SwipableMenu() {}
	virtual void Draw(Renderer& r, Vector2F offset, float alpha) const;
	virtual void Update(Engine& engine, float tick);
	virtual bool OnClick(Engine& engine, const Vector2F& currentPos, MouseButton button);
	virtual bool OnMouseMove(Engine& engine, const Vector2F& currentPos, const Vector2F& dragStartPoint, MouseMoveState state);

	void Add(int viewNo, MenuItem::Pointer p);
	size_t ViewCount() const;
	size_t ItemCount(int viewNo) const;
	MenuItem::Pointer GetItem(int viewNo, int itemNo) const;
	MenuItem::Pointer GetItem(int viewNo, int itemNo);
	void Claer();

	typedef std::vector<MenuItem::Pointer> ViewType;
	std::vector<ViewType> viewList;
	int currentView;
	float moveX;
	float accel;
	bool hasDragging;
	SwipeEventHandler swipeEventHandler;
  };

  /** Menu container.

    Usually, this is used as the root of menu.
  */
  struct Menu : public MenuItem {
	Menu();
	virtual ~Menu() {}
	virtual void Draw(Renderer& r, Vector2F offset, float alpha) const;
	virtual void Update(Engine& engine, float tick);
	virtual bool OnClick(Engine& engine, const Vector2F& currentPos, MouseButton button);
	virtual bool OnMouseButtonDown(Engine& engine, const Vector2F& currentPos, MouseButton button);
	virtual bool OnMouseButtonUp(Engine& engine, const Vector2F& currentPos, MouseButton button);
	virtual bool OnMouseMove(Engine& engine, const Vector2F& currentPos, const Vector2F& startPos, MouseMoveState);

	bool ProcessWindowEvent(Engine& engine, const Event& e);
	void Add(MenuItem::Pointer p);
	void Clear();

	Vector2F pos;
	std::vector<MenuItem::Pointer> items;
	Vector2F dragStartPoint;
	float inputDisableTimer;
	MouseMoveState mouseMoveState;
	MenuItem::Pointer pActiveItem;
  };

} // namespace Menu

} // namespace Mai

#endif // MAI_COMMON_MENU_H_INCLUDED