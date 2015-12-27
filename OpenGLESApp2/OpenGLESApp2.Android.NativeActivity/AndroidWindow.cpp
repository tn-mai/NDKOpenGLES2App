#include "AndroidWindow.h"

namespace Mai {
  AndroidWindow::AndroidWindow(android_app* p) : state(p) {}

  EGLNativeWindowType AndroidWindow::GetWindowType() const {
	return state->window;
  }
  EGLNativeDisplayType AndroidWindow::GetDisplayType() const {
	return 0;
  }
  void AndroidWindow::MessageLoop() {}
  bool AndroidWindow::Initialize(const char* name, size_t width, size_t height) {

  }
  void AndroidWindow::Destroy() {

  }
} // namespace Mai
