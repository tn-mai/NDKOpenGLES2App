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

  /** A moving state of mouse.
  */
  enum class MouseMoveState {
	Begin, ///< Begin mouse cursor moving.
	Moving, ///< Mouse cursor continues to move.
	End, ///< End mouse cursor moving.
  };

  /** The base class of any menu item.
  */
  struct MenuItem {
	typedef std::shared_ptr<MenuItem> Pointer;
	typedef std::function<bool(const Vector2F&, MouseButton)> ClickEventHandler;
	typedef std::function<bool(const Vector2F&, const Vector2F&, MouseMoveState)> MoveEventHandler;

	MenuItem();
	virtual ~MenuItem() = 0;
	virtual void Draw(Renderer&, Vector2F, float) const {}
	virtual void Update(float) {}
	virtual bool OnClick(const Vector2F& currentPos, MouseButton button);
	virtual bool OnMouseMove(const Vector2F& currentPos, const Vector2F& startPos, MouseMoveState);

	void SetRegion(const Vector2F& pos, const Vector2F& size);
	bool OnRegion(const Vector2F& pos) const;
	float GetAlpha() const;

	Vector2F lt, wh; ///< Active area.
	ClickEventHandler clickHandler;
	MoveEventHandler mouseMoveHandler;
	bool isActive;
  };
  inline MenuItem::~MenuItem() = default;

  /** A text menu item.
  */
  struct TextMenuItem : public MenuItem {
	static const uint8_t FLAG_ZOOM_ANIMATION = 0x01;
	static const uint8_t FLAG_ALPHA_ANIMATION = 0x02;
	static const uint8_t FLAG_SHADOW = 0x04;

	TextMenuItem(const char* str, const Vector2F& p, float s, int flg = FLAG_SHADOW);
	TextMenuItem(const char* str, const Vector2F& p, float s, Color4B c, int flg = FLAG_SHADOW);
	virtual ~TextMenuItem() {}
	virtual void Draw(Renderer& r, Vector2F offset, float alpha) const;
	virtual void Update(float tick);
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
	virtual void Update(float tick);
	virtual bool OnClick(const Vector2F& currentPos, MouseButton button);
	virtual bool OnMouseMove(const Vector2F& currentPos, const Vector2F& dragStartPoint, MouseMoveState);

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
	virtual void Update(float tick);
	virtual bool OnClick(const Vector2F& currentPos, MouseButton button);
	virtual bool OnMouseMove(const Vector2F& currentPos, const Vector2F& dragStartPoint, MouseMoveState state);

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
	virtual void Update(float tick);
	virtual bool OnClick(const Vector2F& currentPos, MouseButton button);
	virtual bool OnMouseMove(const Vector2F& currentPos, const Vector2F& startPos, MouseMoveState);

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