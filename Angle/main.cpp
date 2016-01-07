#include "Win32Window.h"
#include "../Common/File.h"
#include "../Common/Engine.h"

struct Camera {
  Camera(const Mai::Position3F& pos, const Mai::Vector3F& at, const Mai::Vector3F& up)
	: position(pos)
	, eyeVector(at)
	, upVector(up)
  {}

  Mai::Position3F position;
  Mai::Vector3F eyeVector;
  Mai::Vector3F upVector;
};

int main() {
  Mai::Win32Window  window;
  Mai::Engine engine(&window);
  window.Initialize("Sunny Side Up", 480, 800);
  window.SetVisible(true);

  Mai::FileSystem::Initialize();

  engine.InitDisplay();

  Camera camera(Mai::Position3F(0, 0, 0), Mai::Vector3F(0, 0, -1), Mai::Vector3F(0, 1, 0));
  int mouseX = -1, mouseY = -1;
  for (;;) {
	window.MessageLoop();
	while (auto e = window.PopEvent()) {
	  switch (e->Type) {
	  case Event::EVENT_CLOSED:
		engine.TermDisplay();
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
	engine.DrawFrame(camera.position, camera.eyeVector, camera.upVector);
  }
  return 0;
}