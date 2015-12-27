#include "Win32Window.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include "../Common/File.h"
#include <string>

int main() {
  Mai::Win32Window  window;
  window.Initialize("Sunny Side Up", 480, 800);
  window.SetVisible(true);

  Mai::FileSystem::Initialize();
  Mai::Renderer renderer;
  renderer.Initialize(window);

  for (;;) {
	while (auto e = window.PopEvent()) {
	  if (e->Type == Event::EVENT_CLOSED) {
		exit(0);
	  }
	  if (renderer.HasDisplay()) {
		renderer.Render(nullptr, nullptr);
		renderer.Swap();
	  }
	  window.MessageLoop();
	}
  }

  return 0;
}