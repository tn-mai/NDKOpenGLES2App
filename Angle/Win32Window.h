#ifndef MAI_WIN32WINDOW_H_INCLUDED
#define MAI_WIN32WINDOW_H_INCLUDED
#include "../Shared/Window.h"
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

	virtual bool SaveUserFile(const char* filename, const void* data, size_t size) const;
	virtual size_t GetUserFileSize(const char* filename) const;
	virtual bool LoadUserFile(const char* filename, void* data, size_t size) const;
	virtual bool DeleteUserFile(const char* filename) const;

  public:
	void IsMouseInWindow(bool b) { isMouseInWindow = b; }
	bool IsMouseInWindow() const { return isMouseInWindow; }
	
	struct MouseInfo {
	  int16_t x;
	  int16_t y;
	  int64_t time;
	};
	void SetMouseInfo(int x, int y, int64_t t) {
	  prevMousePos.x = x;
	  prevMousePos.y = y;
	  prevMousePos.time = t;
	}
	const MouseInfo& GetPrevMouseInfo() const { return prevMousePos; }

  private:
	std::string parentClassName;
	std::string childClassName;
	EGLNativeWindowType nativeWindow;
	EGLNativeWindowType parentWindow;
	EGLNativeDisplayType nativeDisplay;
	MouseInfo prevMousePos;
	bool isMouseInWindow;
  };

} // namespace Mai

#endif // MAI_WIN32WINDOW_H_INCLUDED