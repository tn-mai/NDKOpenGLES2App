/** @file Menu.cpp
*/
#include "Menu.h"
#include "Window.h"
#include "Engine.h"
#include "Event.h"
#include "Audio.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include <boost/math/constants/constants.hpp>
#include <algorithm>

#ifndef NDEBUG
#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "Mai::Menu", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "Mai::Menu", __VA_ARGS__))
#else
#include <stdio.h>
#define LOGI(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#define LOGE(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#endif // __ANDROID__
#else
#define LOGI(...)
#define LOGE(...)
#endif // NDEBUG

namespace Mai {

namespace Menu {

  const char SEID_PageFeed[] = "menu.pf";
  const char SEID_Carousel[] = "menu.cal";
  const char SEID_Confirm[] = "menu.yes";
  const char SEID_Cancel[] = "menu.no";

  /**
	Initialize the menu system.

	@param engine The engine object.
  */
  void Initialize(Engine& engine) {
	AudioInterface& audio = engine.GetAudio();
	audio.LoadSE(SEID_PageFeed, "Audio/pagefeed.wav");
	audio.LoadSE(SEID_Carousel, "Audio/carousel.wav");
	audio.LoadSE(SEID_Confirm, "Audio/confirm.wav");
  }

  /**
	Finalize the menu system.

	@param engine The engine object.
  */
  void Finalize(Engine& engine) {
	(void)engine;
  }
  
  /** Transform the screen space to the range of 0-1.

    @param x  A position of X(pixel).
	@param y  A position of Y(pixel).
	@param w  A screen width(pixel).
	@param h  A screen height(pixel).

	@return 0-1 ranged position transformed from x and y.
  */
  Vector2F GetDeviceIndependentPositon(int x, int y, int w, int h) {
	return Vector2F(static_cast<float>(x) / static_cast<float>(w), static_cast<float>(y) / static_cast<float>(h));
  }

  /** Constructor.
  */
  MenuItem::MenuItem()
	: lt(0, 0), wh(0, 0), isActive(true)
  {
  }

  /** Handle a click event.

    @param currentPosition  A position that was used by a click.
	@param button           A button type that was used by a click.

	@retval true  The event was consumed.
	@retval false The event was not consumed.
  */
  bool MenuItem::OnClick(Engine& engine, const Vector2F& currentPos, MouseButton button) {
	if (clickHandler) {
	  if (clickHandler(currentPos, button)) {
		engine.GetAudio().PlaySE(SEID_Confirm, 1.0f);
		return true;
	  }
	}
	return false;
  }

  /** Handle a move event.

	@param currentPos  A current mouse cursor or swiping position.
	@param startPos    A position of start dragging.
	@param state       A state of mouse moving.

	@retval true  The event was consumed.
	@retval false The event was not consumed.
  */
  bool MenuItem::OnMouseMove(const Vector2F& currentPos, const Vector2F& startPos, MouseMoveState state) {
	if (mouseMoveHandler) {
	  return mouseMoveHandler(currentPos, startPos, state);
	}
	return false;
  }

  /** Set a responding region.

    @param  pos  The left-top position of a responding region.
	@param  size The width-height of a responding region.
  */
  void MenuItem::SetRegion(const Vector2F& pos, const Vector2F& size) {
	lt = pos;
	wh = size;
  }

  /** Text that a point is within the responding region.

    @param  pos  The tested position.

	@retval true  pos is within the responding region.
	@retval false pos is out of the responding region.
  */
  bool MenuItem::OnRegion(const Vector2F& pos) const {
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

  /**  Get transparency value by the state.

    @return A tranparency value.
  */
  float MenuItem::GetAlpha() const { return isActive ? 1.0f : 0.25f; };

  /** Constructor.

    @param str A text for showing.
	@param p   A center position of this object.
	@param s   A base scale of text.
	@param flg The logical sum of FLAG.
  */
  TextMenuItem::TextMenuItem(const char* str, const Vector2F& p, float s, int flg)
	: pos(p), color(240, 240, 240, 255), baseScale(s), scaleTick(0), flags(flg)
  {
	SetText(str);
  }

  /** Constructor.

	@param str A text for showing.
	@param p   A center position of this object.
	@param s   A base scale of text.
	@param c   A color of text.
	@param flg The logical sum of FLAG.
  */
  TextMenuItem::TextMenuItem(const char* str, const Vector2F& p, float s, Color4B c, int flg)
	: pos(p), color(c), baseScale(s), scaleTick(0), flags(flg)
  {
	SetText(str);
  }

  /** Render the object.

    @param  r       A renderer object.
	@param  offset  A rendering offset.
	@param  alpha   A text transparency.
  */
  void TextMenuItem::Draw(Renderer& r, Vector2F offset, float alpha) const {
	offset += pos;
	float scale = 1.0f;
	if (flags & FLAG_ZOOM_ANIMATION) {
	  if (scaleTick < 1.0f) {
		scale = 1.0f + scaleTick * 0.5f;
	  } else {
		scale = 1.5f - (scaleTick - 1.0f) * 0.5f;
	  }
	}
	scale *= baseScale;
	const float w = r.GetStringWidth(label) * scale * 0.5f;
	Color4B c = color;
	if (flags & FLAG_ALPHA_ANIMATION) {
	  if (scaleTick < 1.0f) {
		c.a = static_cast<uint8_t>(c.a * scaleTick);
	  } else {
		c.a = static_cast<uint8_t>(c.a * (2.0f - scaleTick));
	  }
	}
	c.a = static_cast<uint8_t>(c.a * alpha * GetAlpha());
	r.AddString(offset.x - w, offset.y, scale, c, label, flags & FLAG_SHADOW ? Renderer::FONTOPTION_DROPSHADOW : Renderer::FONTOPTION_NONE);
  }

  /** Update a object and children.

	@param engine The engine object.
	@param tick   Second from previous frame.
  */
  void TextMenuItem::Update(Engine& engine, float tick) {
	(void)engine;
	if (flags & (FLAG_ZOOM_ANIMATION | FLAG_ALPHA_ANIMATION)) {
	  scaleTick += tick;
	  if (scaleTick > 2.0f) {
		scaleTick -= 2.0f;
	  }
	}
  }

  /** Set text.

	@param str A text for showing.
  */
  void TextMenuItem::SetText(const char* str) {
	const size_t len = std::min(sizeof(label) - 1, strlen(str));
	std::copy(str, str + len, label);
	label[len] = '\0';
	wh = Vector2F(static_cast<float>(len) * 0.1f * baseScale, baseScale * 0.1f);
	lt = pos - wh * 0.5f;
  }

  /** Get text.

	@return The text striing.
  */
  const char* TextMenuItem::GetText() const { return label; }

  /** Get the flags.

	@return The flags that is logical sum of some flags.
  */
  int TextMenuItem::GetFlag() const { return flags; }

  /** Set the flags.

	@param f  The flags that is logical sum of some flags.
			  The object will have a logical sum of this and object flags.
  */
  void TextMenuItem::SetFlag(int f) { flags |= f; }

  /** Clear specified flags.

	@param f  The flags that is logical sum of some flags.
			  The object will have the flags that is reset this.
  */
  void TextMenuItem::ClearFlag(int f) {
	flags &= ~f;
	if (!(flags & (FLAG_ZOOM_ANIMATION | FLAG_ALPHA_ANIMATION))) {
	  scaleTick = 0.0f;
	}
  }

  /** Constructor.

	@param p    A display position.
	@param size A line size of the window.
	@param top  A top index of items that is showing in the window.
	@param s    A font scale.
  */
  CarouselMenu::CarouselMenu(const Vector2F& p, int size, int top, float s)
	: pos(p), windowSize(size), topOfWindow(top), prevOffset(INT_MAX), scale(s), moveY(0), hasDragging(false)
  {
	SetRegion(p + Vector2F(-0.5f, -0.3f), Vector2F(1.0f, 0.6f));
  }

  /** Render the object.

	@param  r       A renderer object.
	@param  offset  A rendering offset.
	@param  alpha   A text transparency.
  */
  void CarouselMenu::Draw(Renderer& r, Vector2F offset, float alpha) const {
	offset += pos;
	alpha *= GetAlpha();
	for (auto& e : renderingList) {
	  e->Draw(r, offset, alpha);
	}
  }

  /** Update a object and children.

	@param engine The engine object.
	@param tick   Second from previous frame.
  */
  void CarouselMenu::Update(Engine& engine, float tick) {
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

	if (prevOffset != INT_MAX && prevOffset != topOfWindow - indexOffset) {
	  engine.GetAudio().PlaySE(SEID_Carousel, 1.0f);
	  if (changeEventHandler) {
		changeEventHandler(prevOffset, topOfWindow - indexOffset);
	  }
	}
	prevOffset = topOfWindow - indexOffset;

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
	  pItem->baseScale = (alpha * alpha) * scale;
	  pItem->pos.y = std::sin(theta) * 0.15f * scale;
	  renderingList.push_back(pItem);
	}
	std::sort(
	  renderingList.begin(),
	  renderingList.end(),
	  [](const MenuItem::Pointer& lhs, const MenuItem::Pointer& rhs) {
		return static_cast<const TextMenuItem*>(lhs.get())->baseScale < static_cast<const TextMenuItem*>(rhs.get())->baseScale;
	  }
	);

	for (auto& e : items) {
	  e->Update(engine, tick);
	}
  }

  /** Handle a click event.

	@param currentPosition  A position that was used by a click.
	@param button           A button type that was used by a click.

	@retval true  The event was consumed.
	@retval false The event was not consumed.
  */
  bool CarouselMenu::OnClick(Engine& engine, const Vector2F& currentPos, MouseButton button) {
	const int containerSize = static_cast<int>(items.size());
	const int center = windowSize / 2;
	return items[(topOfWindow + center) % containerSize]->OnClick(engine, currentPos, button);
	return false;
  }

  /** Handle a move event.

	@param currentPos  A current mouse cursor or swiping position.
	@param startPos    A position of start dragging.
	@param state       A state of mouse moving.

	@retval true  The event was consumed.
	@retval false The event was not consumed.
  */
  bool CarouselMenu::OnMouseMove(const Vector2F& currentPos, const Vector2F& dragStartPoint, MouseMoveState state) {
	switch (state) {
	case MouseMoveState::Begin:
	  moveY = currentPos.y - dragStartPoint.y;
	  hasDragging = true;
	  break;
	case MouseMoveState::Moving:
	  moveY = currentPos.y - dragStartPoint.y;
	  hasDragging = true;
	  break;
	case MouseMoveState::End: {
	  const int containerSize = static_cast<int>(items.size());
	  topOfWindow = (topOfWindow - static_cast<int>(moveY * 10.0f) + containerSize) % containerSize;
	  hasDragging = false;
	  moveY *= 10.0f;
	  moveY -= static_cast<float>(static_cast<int>(moveY));
	  moveY *= 0.1f;
	  break;
	}
	}
	return true;
  }

  /** Add a menu item.

	@param p  A menu item that is added to this object.
  */
  void CarouselMenu::Add(MenuItem::Pointer p) { items.push_back(p); }

  /** Clear all menu item.
  */
  void CarouselMenu::Clear() {
	renderingList.clear();
	items.clear();
  }

  /** Constructor.
  */
  SwipableMenu::SwipableMenu(size_t viewCount)
	: currentView(0)
	, moveX(0)
	, accel(0)
	, hasDragging(false)
  {
	viewList.resize(viewCount);
  }

  /** Render the object.

	@param  r       A renderer object.
	@param  offset  A rendering offset.
	@param  alpha   A text transparency.
  */
  void SwipableMenu::Draw(Renderer& r, Vector2F offset, float alpha) const {
	{
	  const Vector2F tmpOffset = offset + Vector2F(moveX, 0);
	  alpha *= GetAlpha();
	  for (auto& e : viewList[currentView]) {
		e->Draw(r, tmpOffset, alpha);
	  }
	}
	const int viewCount = static_cast<int>(viewList.size());
	{
	  const Vector2F tmpOffset = offset + Vector2F(moveX - 0.85f, 0);
	  const int leftView = (currentView - 1 + viewCount) % viewCount;
	  for (auto& e : viewList[leftView]) {
		e->Draw(r, tmpOffset, alpha);
	  }
	}
	{
	  const Vector2F tmpOffset = offset + Vector2F(moveX + 0.85f, 0);
	  const int rightView = (currentView + 1) % viewCount;
	  for (auto& e : viewList[rightView]) {
		e->Draw(r, tmpOffset, alpha);
	  }
	}
  }

  /** Update a object and children.

	@param engine The engine object.
	@param tick   Second from previous frame.
  */
  void SwipableMenu::Update(Engine& engine, float tick) {
	for (auto& e : viewList[currentView]) {
	  e->Update(engine, tick);
	}
	const float springForce = 1.0f * tick;
	if (!hasDragging && (moveX || accel)) {
	  if (moveX * accel >= 0) {
		accel += accel >= 0 ? -springForce : springForce;
	  } else {
		accel += accel >= 0 ? springForce : -springForce;
	  }
	  const float newX = moveX + accel;
	  if (newX * moveX >= 0) {
		moveX = newX;
	  } else {
		moveX = accel = 0;
	  }
	  const int prevView = currentView;
	  if (moveX > 0.4f) {
		moveX -= 0.81f + accel;
		accel += springForce;
		const int viewCount = static_cast<int>(ViewCount());
		currentView = (currentView - 1 + viewCount) % viewCount;
	  } else if (moveX < -0.4f) {
		moveX += 0.81f - accel;
		accel -= springForce;
		const int viewCount = static_cast<int>(ViewCount());
		currentView = (currentView + 1) % viewCount;
	  }
	  if (prevView != currentView) {
		engine.GetAudio().PlaySE(SEID_PageFeed, 1.0f);
		if (swipeEventHandler) {
		  swipeEventHandler(prevView, currentView);
		}
	  }
	}
  }

  /** Handle a click event.

	@param currentPosition  A position that was used by a click.
	@param button           A button type that was used by a click.

	@retval true  The event was consumed.
	@retval false The event was not consumed.
  */
  bool SwipableMenu::OnClick(Engine& engine, const Vector2F& currentPos, MouseButton button) {
	const auto re = viewList[currentView].rend();
	for (auto ri = viewList[currentView].rbegin(); ri != re; ++ri) {
	  if ((*ri)->OnRegion(currentPos)) {
		return (*ri)->OnClick(engine, currentPos, button);
	  }
	}
	return false;
  }

  /** Handle a move event.

	@param currentPos  A current mouse cursor or swiping position.
	@param startPos    A position of start dragging.
	@param state       A state of mouse moving.

	@retval true  The event was consumed.
	@retval false The event was not consumed.
  */
  bool SwipableMenu::OnMouseMove(const Vector2F& currentPos, const Vector2F& dragStartPoint, MouseMoveState state) {
	switch (state) {
	case MouseMoveState::Begin:
	  /* FALLTHROUGH */
	case MouseMoveState::Moving:
	  accel = accel * 0.25f + (currentPos.x - dragStartPoint.x) - moveX;
	  moveX = currentPos.x - dragStartPoint.x;
	  hasDragging = true;
	  break;
	case MouseMoveState::End:
	  accel = accel * 0.25f + (currentPos.x - dragStartPoint.x) - moveX;
	  moveX = currentPos.x - dragStartPoint.x;
	  hasDragging = false;
	  break;
	}
	return true;
  }

  /** Add a menu item.

	@param p  A menu item that is added to this object.
  */
  void SwipableMenu::Add(int viewNo, MenuItem::Pointer p) {
	if (viewNo >= 0 && viewNo < static_cast<int>(viewList.size())) {
	  viewList[viewNo].push_back(p);
	}
  }

  /** Clear all menu item.
  */
  void SwipableMenu::Claer() {
	viewList.clear();
  }

  /** Get a view count.

	@return The count of view.
  */
  size_t SwipableMenu::ViewCount() const { return viewList.size(); }

  /** Get a item count in the view.

	@param  viewNo  A target view number.
	                It is less than ViewCount();

	@return The count of item.
  */
  size_t SwipableMenu::ItemCount(int viewNo) const {
	if (viewNo < 0 || viewNo >= static_cast<int>(viewList.size())) {
	  return 0;
	}
	return viewList[viewNo].size();
  }

  /** Get a item count in the view.

	@param  viewNo  A target view number.
	                It is less than ViewCount();
	@param  itemNo  A target item number.
					It is less than ItemCount();

	@return A pointer object to the item.
  */
  MenuItem::Pointer SwipableMenu::GetItem(int viewNo, int itemNo) const {
	if (viewNo < 0 || viewNo >= static_cast<int>(viewList.size())) {
	  return MenuItem::Pointer();
	}
	const ViewType& view = viewList[viewNo];
	if (itemNo < 0 || itemNo >= static_cast<int>(view.size())) {
	  return MenuItem::Pointer();
	}
	return view[itemNo];
  }

  /** Get a item count in the view.

	@param  viewNo  A target view number.
	                It is less than ViewCount();
	@param  itemNo  A target item number.
	                It is less than ItemCount();

	@return A pointer object to the item.
  */
  MenuItem::Pointer SwipableMenu::GetItem(int viewNo, int itemNo) {
	return const_cast<const SwipableMenu*>(this)->GetItem(viewNo, itemNo);
  }

  /** Constructor.
  */
  Menu::Menu()
	: pos(0, 0)
	, inputDisableTimer(0)
	, mouseMoveState(MouseMoveState::End)
  {
	SetRegion(Vector2F(0, 0), Vector2F(1, 1));
  }

  /** Render the object.

	@param  r       A renderer object.
	@param  offset  A rendering offset.
	@param  alpha   A text transparency.
  */
  void Menu::Draw(Renderer& r, Vector2F offset, float alpha) const {
	offset += pos;
	alpha *= GetAlpha();
	for (auto& e : items) {
	  e->Draw(r, offset, alpha);
	}
  }

  /** Update a object and children.

	@param engine The engine object.
	@param tick   Second from previous frame.
  */
  void Menu::Update(Engine& engine, float tick) {
	for (auto& e : items) {
	  e->Update(engine, tick);
	}
	if (inputDisableTimer > 0.0f) {
	  inputDisableTimer = std::max(0.0f, inputDisableTimer - tick);
	}
  }

  /** Handle a click event.

	@param currentPosition  A position that was used by a click.
	@param button           A button type that was used by a click.

	@retval true  The event was consumed.
	@retval false The event was not consumed.
  */
  bool Menu::OnClick(Engine& engine, const Vector2F& currentPos, MouseButton button) {
	const auto re = items.rend();
	for (auto ri = items.rbegin(); ri != re; ++ri) {
	  if ((*ri)->OnRegion(currentPos)) {
		return (*ri)->OnClick(engine, currentPos, button);
	  }
	}
	return false;
  }

  /** Handle a move event.

	@param currentPos  A current mouse cursor or swiping position.
	@param startPos    A position of start dragging.
	@param state       A state of mouse moving.

	@retval true  The event was consumed.
	@retval false The event was not consumed.
  */
  bool Menu::OnMouseMove(const Vector2F& currentPos, const Vector2F& startPos, MouseMoveState state) {
	const auto re = items.rend();
	for (auto ri = items.rbegin(); ri != re; ++ri) {
	  if ((*ri)->OnRegion(currentPos)) {
		return (*ri)->OnMouseMove(currentPos, startPos, state);
	  }
	}
	return false;
  }

  /** Process the event sent from the window.

	@param engine The engine object.
	@param e      The window event.

	@retval true  The event was processed.
	@retval false The event was ignored.
  */
  bool Menu::ProcessWindowEvent(Engine& engine, const Event& e) {
	if (inputDisableTimer > 0.0f) {
	  return false;
	}
	const int windowWidth = engine.GetRenderer().Width();
	const int windowHeight = engine.GetRenderer().Height();
	switch (e.Type) {
	case Event::EVENT_MOUSE_MOVED:
	  if (pActiveItem) {
		const Vector2F currentPos = GetDeviceIndependentPositon(e.MouseMove.X, e.MouseMove.Y, windowWidth, windowHeight);
		switch (mouseMoveState) {
		case MouseMoveState::Begin: mouseMoveState = MouseMoveState::Moving; break;
		case MouseMoveState::Moving: mouseMoveState = MouseMoveState::Moving; break;
		case MouseMoveState::End: mouseMoveState = MouseMoveState::Begin; break;
		}
		return MenuItem::Pointer(pActiveItem)->OnMouseMove(currentPos, dragStartPoint, mouseMoveState);
	  }
	  break;
	case Event::EVENT_MOUSE_BUTTON_PRESSED: {
	  LOGI("EVENT_MOUSE_BUTTON_PRESSED(%d,%d) (%d,%d)", e.MouseButton.X, e.MouseButton.Y, windowWidth, windowHeight);
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
	case Event::EVENT_MOUSE_BUTTON_CLICKED: {
	  const Vector2F currentPos = GetDeviceIndependentPositon(e.MouseButton.X, e.MouseButton.Y, windowWidth, windowHeight);
	  if (pActiveItem) {
		if (MenuItem::Pointer(pActiveItem)->OnClick(engine, currentPos, e.MouseButton.Button)) {
		  return true;
		}
	  }
	  if (OnRegion(currentPos)) {
		const auto re = items.rend();
		for (auto ri = items.rbegin(); ri != re; ++ri) {
		  if ((*ri)->OnRegion(currentPos)) {
			return MenuItem::Pointer(*ri)->OnClick(engine, currentPos, e.MouseButton.Button);
		  }
		}
	  }
	  break;
	}
	case Event::EVENT_MOUSE_BUTTON_RELEASED: {
	  LOGI("EVENT_MOUSE_BUTTON_RELEASED(%d,%d) (%d,%d)", e.MouseButton.X, e.MouseButton.Y, windowWidth, windowHeight);
	  const Vector2F currentPos = GetDeviceIndependentPositon(e.MouseButton.X, e.MouseButton.Y, windowWidth, windowHeight);
	  if (pActiveItem) {
		MenuItem::Pointer p(std::move(pActiveItem));
		mouseMoveState = MouseMoveState::End;
		if (p->OnMouseMove(currentPos, dragStartPoint, mouseMoveState)) {
		  return true;
		}
	  }
	  break;
	}
	}
	return false;
  }

  /** Add a menu item.

	@param p  A menu item that is added to this object.
  */
  void Menu::Add(MenuItem::Pointer p) { items.push_back(p); }

  /** Clear all menu item.
  */
  void Menu::Clear() {
	pActiveItem.reset();
	items.clear();
	mouseMoveState = MouseMoveState::End;
  }

} // namespace Menu

} // namespace Mai
