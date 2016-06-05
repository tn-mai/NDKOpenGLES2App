/** @file Menu.cpp
*/
#include "Menu.h"
#include "Window.h"
#include "Engine.h"
#include "Event.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include <boost/math/constants/constants.hpp>

namespace Mai {

namespace Menu {

  Vector2F GetDeviceIndependentPositon(int x, int y, int w, int h) {
	return Vector2F(static_cast<float>(x) / static_cast<float>(w), static_cast<float>(y) / static_cast<float>(h));
  }

  bool MenuItem::OnClick(const Vector2F& currentPos, MouseButton button) {
	if (clickHandler) {
	  return clickHandler(currentPos, button);
	}
	return false;
  }

  bool MenuItem::OnMouseMove(const Vector2F& currentPos, const Vector2F& startPos) {
	if (mouseMoveHandler) {
	  return mouseMoveHandler(currentPos, startPos);
	}
	return false;
  }

  void MenuItem::SetRegion(const Vector2F& pos, const Vector2F& size) {
	lt = pos;
	wh = size;
  }

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
  float MenuItem::GetAlpha() const { return isActive ? 1.0f : 0.25f; };

  TextMenuItem::TextMenuItem(const char* str, const Vector2F& p, float s, int flg)
	: pos(p), baseScale(s), scaleTick(0), flags(flg)
  {
	const size_t len = std::min(sizeof(label) - 1, strlen(str));
	std::copy(str, str + len, label);
	label[len] = '\0';
	wh = Vector2F(static_cast<float>(len) * 0.1f * baseScale, baseScale * 0.1f);
	lt = pos - wh * 0.5f;
	color = Color4B(240, 240, 240, static_cast<uint8_t>(255.0f * s));
  }

  void TextMenuItem::Draw(Renderer& r, Vector2F offset, float alpha) const {
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
	if (flags & FLAG_SHADOW) {
	  const Color4B shadowColor(c.r / 4, c.g / 4, c.b / 4, c.a / 2);
	  r.AddString(offset.x - w + 0.0075f, offset.y + 0.01f, scale, shadowColor, label);
	}
	r.AddString(offset.x - w, offset.y, scale, c, label);
  }

  void TextMenuItem::Update(float tick) {
	if (flags & FLAG_ZOOM_ANIMATION) {
	  scaleTick += tick;
	  if (scaleTick > 2.0f) {
		scaleTick -= 2.0f;
	  }
	}
  }

  int TextMenuItem::GetFlag() const { return flags; }
  void TextMenuItem::SetFlag(int f) { flags |= f; }
  void TextMenuItem::ClearFlag(int f) {
	flags &= ~f;
	if (!(flags & FLAG_ZOOM_ANIMATION)) {
	  scaleTick = 0.0f;
	}
  }

  CarouselMenu::CarouselMenu(const Vector2F& p, int size, int top, float s)
	: pos(p), windowSize(size), topOfWindow(top), scale(s), moveY(0), hasDragging(false)
  {
	SetRegion(Vector2F(0, 0), Vector2F(1, 1));
  }

  void CarouselMenu::Draw(Renderer& r, Vector2F offset, float alpha) const {
	offset += pos;
	alpha *= GetAlpha();
	for (auto& e : renderingList) {
	  e->Draw(r, offset, alpha);
	}
  }

  void CarouselMenu::Update(float tick) {
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
	  pItem->baseScale = (alpha * alpha) * scale;
	  pItem->pos.y = std::sin(theta) * 0.25f;
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
	  e->Update(tick);
	}
  }

  bool CarouselMenu::OnClick(const Vector2F& currentPos, MouseButton button) {
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

  bool CarouselMenu::OnMouseMove(const Vector2F& currentPos, const Vector2F& dragStartPoint) {
	moveY = currentPos.y - dragStartPoint.y;
	hasDragging = true;
	return true;
  }

  void CarouselMenu::Add(MenuItem::Pointer p) { items.push_back(p); }
  void CarouselMenu::Clear() {
	renderingList.clear();
	items.clear();
  }

  Menu::Menu()
	: pos(0, 0)
  {
	SetRegion(Vector2F(0, 0), Vector2F(1, 1));
  }

  void Menu::Draw(Renderer& r, Vector2F offset, float alpha) const {
	offset += pos;
	alpha *= GetAlpha();
	for (auto& e : items) {
	  e->Draw(r, offset, alpha);
	}
  }

  void Menu::Update(float tick) {
	for (auto& e : items) {
	  e->Update(tick);
	}
  }

  bool Menu::OnClick(const Vector2F& currentPos, MouseButton button) {
	const auto re = items.rend();
	for (auto ri = items.rbegin(); ri != re; ++ri) {
	  if ((*ri)->OnRegion(currentPos)) {
		return (*ri)->OnClick(currentPos, button);
	  }
	}
	return false;
  }

  bool Menu::OnMouseMove(const Vector2F& currentPos, const Vector2F& startPos) {
	const auto re = items.rend();
	for (auto ri = items.rbegin(); ri != re; ++ri) {
	  if ((*ri)->OnRegion(currentPos)) {
		return (*ri)->OnMouseMove(currentPos, startPos);
	  }
	}
	return false;
  }

  bool Menu::ProcessWindowEvent(Engine& engine, const Event& e) {
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

  void Menu::Add(MenuItem::Pointer p) { items.push_back(p); }
  void Menu::Clear() { items.clear(); }

} // namespace Menu

} // namespace Mai
