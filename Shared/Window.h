#ifndef MAI_WINDOW_H_INCLUDED
#define MAI_WINDOW_H_INCLUDED
#include "Event.h"
#include <EGL/egl.h>
#include <deque>
#include <boost/optional.hpp>

namespace Mai {

class Window {
public:
  virtual ~Window() {}
  virtual EGLNativeWindowType GetWindowType() const = 0;
  virtual EGLNativeDisplayType GetDisplayType() const = 0;
  virtual void MessageLoop() = 0;
  virtual bool Initialize(const char* name, size_t width, size_t height) = 0;
  virtual void Destroy() = 0;
  virtual void SetVisible(bool isVisible) = 0;
  virtual void PushEvent(const Event& e) {
	switch (e.Type) {
	case Event::EVENT_MOVED:
	  x = e.Move.X;
	  y = e.Move.Y;
	  break;
	case Event::EVENT_RESIZED:
	  width = e.Size.Width;
	  height = e.Size.Height;
	  break;
	default:
	  break;
	}
	eventQueue.push_back(e);
  }

public:
  boost::optional<Event> PopEvent() {
	if (eventQueue.empty()) {
	  return boost::none;
	}
	Event e = eventQueue.front();
	eventQueue.pop_front();
	return e;
  }
  int GetX() const { return x; }
  int GetY() const { return y; }
  int GetWidth() const { return width; }
  int GetHeight() const { return height; }

private:
  int x;
  int y;
  int width;
  int height;
  std::deque<Event> eventQueue;
};

} // namespace Mai

#endif // MAI_WINDOW_H_INCLUDED
