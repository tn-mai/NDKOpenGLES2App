#include "Win32Window.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include "../Common/File.h"
#include <string>

int main() {
  Mai::Win32Window  window;
  window.Initialize("Sunny Side Up", 480, 800);
  Mai::FileSystem::Initialize();
  Mai::Renderer renderer;
  renderer.Initialize(window);

  return 0;
}