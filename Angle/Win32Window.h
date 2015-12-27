#ifndef MAI_WIN32WINDOW_H_INCLUDED
#define MAI_WIN32WINDOW_H_INCLUDED
#include "../Common/Window.h"
#include <string>

namespace Mai {

  class Win32Window : public Window {
  public:
	virtual ~Win32Window();
	virtual EGLNativeWindowType GetWindowType() const;
	virtual EGLNativeDisplayType GetDisplayType() const;
	virtual void MessageLoop();
	virtual bool Initialize(const char* name, size_t width, size_t height);
	virtual void Destroy();
	virtual void SetVisible(bool isVisible);
	virtual void PushEvent(const Event&);

  public:
	void IsMouseInWindow(bool b) { isMouseInWindow = b; }
	bool IsMouseInWindow() const { return isMouseInWindow; }

  private:
	std::string parentClassName;
	std::string childClassName;
	EGLNativeWindowType nativeWindow;
	EGLNativeWindowType parentWindow;
	EGLNativeDisplayType nativeDisplay;
	bool isMouseInWindow;
  };

} // namespace Mai

#endif // MAI_WIN32WINDOW_H_INCLUDED