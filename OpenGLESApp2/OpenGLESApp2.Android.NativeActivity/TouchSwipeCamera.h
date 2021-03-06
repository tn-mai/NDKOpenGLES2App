#ifndef TOUCHSWIPECAMERA_H_INCLUDED
#define TOUCHSWIPECAMERA_H_INCLUDED
#include "Renderer.h"
#include <android/input.h>

struct android_app;

namespace Mai {

  class TouchSwipeCamera
  {
  public:
	enum MoveDir {
	  MoveDir_Newtral,
	  MoveDir_Front1,
	  MoveDir_Front2,
	  MoveDir_Back
	};
	TouchSwipeCamera(android_app*);
	~TouchSwipeCamera();

	void ScreenSize(int w, int h) { sw = w; sh = h; }
	void Reset();
	bool HandleEvent(AInputEvent*);
	void Update(float, const Vector3F&);
	void Suspend();
	void Resume();
	void Position(const Position3F&);
	Position3F Position() const;
	void EyeVector(const Vector3F&);
	Vector3F EyeVector() const;
	void UpVector(const Vector3F&);
	Vector3F UpVector() const;
	void LookAt(const Position3F&);
	MoveDir Direction() const { return dir; }
	void Direction(MoveDir d) { dir = d; }

  private:
	int sw, sh;
	Position3F position;
	Vector3F eyeVector;
	Vector3F upVector;
	MoveDir dir;
	float dpFactor;
	bool dragging;
	bool suspending;

	struct TapInfo {
	  int32_t id;
	  float x, y;
	};
	TapInfo tapInfo;
	TapInfo dragInfo;
	Vector2F dragStartPoint;
	Vector2F dragCurrentPoint;
	Vector3F dragStartEyeVector;
	Vector3F dragStartUpVector;
  };

} // namespace Mai

#endif // TOUCHSWIPECAMERA_H_INCLUDED