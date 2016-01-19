#include "Win32Window.h"
#include "../Common/File.h"
#include "../Common/Engine.h"

int main() {
  Mai::Win32Window  window;
  Mai::Engine engine(&window);
  window.Initialize("Sunny Side Up", 480, 800);
  window.SetVisible(true);

  Mai::FileSystem::Initialize();

  engine.InitDisplay();

  for (;;) {
	window.MessageLoop();
	const Mai::Engine::State state = engine.Update(&window, 1.0f / 30.0f);
	if (state == Mai::Engine::STATE_TERMINATE) {
	  break;
	}
	engine.DrawFrame();
  }
  return 0;
}