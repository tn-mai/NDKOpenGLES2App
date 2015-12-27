#include "Win32Window.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include "../Common/File.h"
#include <string>

namespace Mai {

  struct Camera {
	Camera(const Mai::Position3F& pos, const Mai::Vector3F& at, const Mai::Vector3F& up)
	  : position(pos)
	  , eyeVector(at)
	  , upVector(up)
	{}

	Position3F position;
	Vector3F eyeVector;
	Vector3F upVector;
  };

} // namespace Mai

int main() {
  Mai::Win32Window  window;
  window.Initialize("Sunny Side Up", 480, 800);
  window.SetVisible(true);

  Mai::FileSystem::Initialize();
  Mai::Renderer renderer;
  renderer.Initialize(window);

  Mai::Camera camera(Mai::Position3F(0, 0, 0), Mai::Vector3F(0, 0, -1), Mai::Vector3F(0, 1, 0));
  int mouseX = -1, mouseY = -1;
  for (;;) {
	while (auto e = window.PopEvent()) {
	  switch (e->Type) {
	  case Event::EVENT_CLOSED:
		exit(0);
		break;
	  case Event::EVENT_MOUSE_ENTERED:
		mouseX = -1;
		break;
	  case Event::EVENT_MOUSE_MOVED: {
		if (mouseX >= 0) {
		  const float x = static_cast<float>(mouseX - e->MouseMove.X) * 0.005f;
		  const float y = static_cast<float>(mouseY - e->MouseMove.Y) * 0.005f;
		  const Mai::Vector3F leftVector = camera.eyeVector.Cross(camera.upVector).Normalize();
		  camera.eyeVector = (Mai::Quaternion(camera.upVector, x) * Mai::Quaternion(leftVector, y)).Apply(camera.eyeVector).Normalize();
		  camera.upVector = Mai::Quaternion(leftVector, y).Apply(camera.upVector).Normalize();
		}
		mouseX = e->MouseMove.X;
		mouseY = e->MouseMove.Y;
		break;
	  }
	  default:
		break;
	  }
	}
	if (renderer.HasDisplay()) {
	  renderer.Update(0.016f, camera.position, camera.eyeVector, camera.upVector);
	  renderer.Render(nullptr, nullptr);
	  renderer.Swap();
	}
	window.MessageLoop();
  }
  return 0;
}